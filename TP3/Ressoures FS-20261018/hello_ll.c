/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello_ll.c `pkg-config fuse --cflags --libs` -o hello_ll
*/

#define FUSE_USE_VERSION 26

#include <fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "tosfs.h"

static void *fs_init; // pointer to the filesystem image
static size_t fs_size;
static struct tosfs_superblock *sb; // pointer to the superblock


// function to check if a block is referenced by any inode in the inode cache: avoid using a block that is already used by another file when creating a new file
static int block_is_referenced(unsigned block_no)
{
	for (unsigned i = 1; i <= sb->inodes; i++) { 
		if (((sb->inode_bitmap >> i) & 1) && inode_cache[i].block_no == block_no) 
			return 1;
	}
	return 0;
}

static int hello_stat(fuse_ino_t ino, struct stat *stbuf) 
{
	// checks if the inode number is valid
	if (ino==0 || ino> sb->inodes || !((sb->inode_bitmap>>ino)&1)) { 
		return -1;
	}

	// fills the stat structure with the information from the inode cache
	stbuf->st_ino=ino;
	stbuf->st_mode=inode_cache[ino].mode; 
	stbuf->st_nlink=inode_cache[ino].nlink; 
	stbuf->st_uid=inode_cache[ino].uid;
	stbuf->st_gid=inode_cache[ino].gid;
	stbuf->st_size=inode_cache[ino].size;
	return 0;
}

static void hello_ll_getattr(fuse_req_t req, fuse_ino_t ino,
			     struct fuse_file_info *fi) 
{
	struct stat stbuf;
	(void) fi; 

	memset(&stbuf, 0, sizeof(stbuf));
	if (hello_stat(ino, &stbuf) == -1)
		fuse_reply_err(req, ENOENT);
	else 
		fuse_reply_attr(req, &stbuf, 1.0); 
}

// function to lookup a file in the filesystem given its parent inode number and its name
static void hello_ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e; // structure to store the information of the file to be looked up

	// check if the parent inode number is valid (it exists and is a directory)
	if (parent == 0 || parent > sb->inodes || !((sb->inode_bitmap >> parent) & 1) || (inode_cache[parent].mode & S_IFMT) != S_IFDIR) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	// get the dentry cache for the parent directory
	struct tosfs_dentry *dentry_cache = (struct tosfs_dentry *)((char *)fs_init + inode_cache[parent].block_no * TOSFS_BLOCK_SIZE);
	// get the number of dentries in the parent directory
	size_t dentry_count = TOSFS_BLOCK_SIZE / sizeof(*dentry_cache);

	// go through dentry cache to find the file with the given name
	for (size_t i = 0; i < dentry_count; i++) {
		if (dentry_cache[i].inode != 0 && strncmp(name, dentry_cache[i].name, TOSFS_MAX_NAME_LENGTH) == 0) {
			memset(&e, 0, sizeof(e)); 
			e.ino = dentry_cache[i].inode; // set the inode number of the file to be looked up
			e.attr_timeout = 1.0;
			e.entry_timeout = 1.0;
			if (hello_stat(e.ino, &e.attr) == -1) { // check if the inode number of the file to be looked up is valid
				fuse_reply_err(req, ENOENT);
				return;
			}
			fuse_reply_entry(req, &e); // reply with the information of the file to be looked up
			return;
		}
	}
	fuse_reply_err(req, ENOENT);
}

struct dirbuf {
	char *p;
	size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
		       fuse_ino_t ino)
{
	struct stat stbuf;
	size_t oldsize = b->size;
	b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
	b->p = (char *) realloc(b->p, b->size);
	memset(&stbuf, 0, sizeof(stbuf));
	stbuf.st_ino = ino;
	fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
			  b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
			     off_t off, size_t maxsize)
{
	if (off >= 0 && (size_t) off < bufsize)
		return fuse_reply_buf(req, buf + off,
				      min(bufsize - off, maxsize));
	else
		return fuse_reply_buf(req, NULL, 0);
}


// function hello_ll_readdir to read the content of a directory in the filesystem
static void hello_ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
			     off_t off, struct fuse_file_info *fi)
{
	(void) fi;

	// check if the inode number is valid (it exists and is a directory)
	if (ino == 0 || ino > sb->inodes ||
	    !((sb->inode_bitmap >> ino) & 1)) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	// check if the inode number is a directory
	if ((inode_cache[ino].mode & S_IFMT) != S_IFDIR) {
		fuse_reply_err(req, ENOTDIR);
		return;
	}

	// Create a buffer to store the directory entries.
	struct dirbuf b;
	memset(&b, 0, sizeof(b));

	// get the dentry cache for the directory
	struct tosfs_dentry *dentry_cache = (struct tosfs_dentry *)((char *)fs_init + inode_cache[ino].block_no * TOSFS_BLOCK_SIZE);
	size_t dentry_count = TOSFS_BLOCK_SIZE / sizeof(*dentry_cache);
	for (size_t i = 0; i < dentry_count; i++) {
		if (dentry_cache[i].inode != 0) {
			dirbuf_add(req, &b, dentry_cache[i].name, dentry_cache[i].inode);
		}
	}
	reply_buf_limited(req, b.p, b.size, off, size);
	free(b.p);
}


static void hello_ll_open(fuse_req_t req, fuse_ino_t ino,
			  struct fuse_file_info *fi)
{
	// check if inode number is valid (it exists and is a file)
	if (ino == 0 || ino > sb->inodes || !((sb->inode_bitmap >> ino) & 1)) 
		fuse_reply_err(req, ENOENT);
	else if ((inode_cache[ino].mode & S_IFMT) == S_IFDIR) // check if inode number is a directory
		fuse_reply_err(req, EISDIR);
	else // check if inode number is a regular file
		fuse_reply_open(req, fi);
}


static void hello_ll_read(fuse_req_t req, fuse_ino_t ino, size_t size,
			  off_t off, struct fuse_file_info *fi)
{
	(void) fi;
	// check if inode number is valid (exists and is a file)
	if (ino == 0 || ino > sb->inodes || !((sb->inode_bitmap >> ino) & 1)) {
		fuse_reply_err(req, ENOENT);
		return;
	}
	// check if inode number is a regular file (not a directory)
	if ((inode_cache[ino].mode & S_IFMT) != S_IFREG) { 
		fuse_reply_err(req, EISDIR); 
		return;
	}

	// get the data of the file from the filesystem image
	const char *data = (const char *)fs_init + inode_cache[ino].block_no * TOSFS_BLOCK_SIZE;
	reply_buf_limited(req, data, inode_cache[ino].size, off, size);
}

// function to set the attributes of a file in the filesystem given its inode number and the attributes: needed for the functions hello_ll_create and hello_ll_write
static void hello_ll_setattr(fuse_req_t req, fuse_ino_t ino,struct stat *attr, int to_set,struct fuse_file_info *fi)
{
	(void) fi;

	if (hello_stat(ino, attr) == -1) { // check if the inode number is valid 
		fuse_reply_err(req, ENOENT);
		return;
	}

	if (to_set & FUSE_SET_ATTR_SIZE) { // check if the size attribute is to be set
		// check if the inode number is a regular file, size is valid (not negative and not bigger than block size)
		if ((inode_cache[ino].mode & S_IFMT) != S_IFREG || attr->st_size < 0 || (uintmax_t) attr->st_size > TOSFS_BLOCK_SIZE) {
			fuse_reply_err(req, EINVAL);
			return;
		}
		inode_cache[ino].size = (__u16) attr->st_size; // set the size attribute of the inode in the inode cache
	}

	// TOSFS does not persist timestamps, but accepts timestamp-only updates
	if (to_set & ~(FUSE_SET_ATTR_SIZE | FUSE_SET_ATTR_ATIME |
		      FUSE_SET_ATTR_MTIME | FUSE_SET_ATTR_ATIME_NOW |
		      FUSE_SET_ATTR_MTIME_NOW)) {
		fuse_reply_err(req, EOPNOTSUPP);
		return;
	}

	if (hello_stat(ino, attr) == -1) { // check if the inode number is valid after setting the attributes
		fuse_reply_err(req, ENOENT);
		return;
	}
	fuse_reply_attr(req, attr, 1.0); // reply with attributes of the inode after setting
}


// function to create a new file in the filesystem given its parent inode number, its name and its mode
static void hello_ll_create(fuse_req_t req, fuse_ino_t parent, const char *name,mode_t mode, struct fuse_file_info *fi) {
	(void) fi;

	// check if the parent inode number is valid (exists and is a directory)
	if (parent==0 || parent>sb->inodes || !((sb->inode_bitmap>>parent)&1) || (inode_cache[parent].mode & S_IFMT)!=S_IFDIR) {
		fuse_reply_err(req,ENOENT);
		return;
	}

	// check if the name is valid (not empty, not too long)
	if (strlen(name)==0 || strlen(name)>=TOSFS_MAX_NAME_LENGTH) {
		fuse_reply_err(req,EINVAL);
		return;
	}

	// check if the name already exists in the parent directory
	struct tosfs_dentry *dentry_cache=(struct tosfs_dentry *)((char *)fs_init + inode_cache[parent].block_no*TOSFS_BLOCK_SIZE);
	size_t dentry_count=TOSFS_BLOCK_SIZE/sizeof(*dentry_cache);
	for (size_t i=0;i<dentry_count;i++) {
		if (dentry_cache[i].inode!=0 && strncmp(name,dentry_cache[i].name,TOSFS_MAX_NAME_LENGTH)==0) {
			fuse_reply_err(req,EEXIST);
			return;
		}
	}

	// find a free inode in the inode bitmap
	int new_inode=-1;
	for (unsigned i=1; i<=sb->inodes; i++) {
		if (!((sb->inode_bitmap>>i)&1)) {
			new_inode=(int)i;
			break;
		}
	}
	if (new_inode==-1) { // if no free inode, reply with error no space left
		fuse_reply_err(req,ENOSPC);
		return;
	}

	// find a free block in the block bitmap
	int new_block=-1;
	for (unsigned i=0; i<sb->blocks; i++) {
		if (!((sb->block_bitmap>>i)&1) && !block_is_referenced(i)) { // check if the block is free and not referenced by any inode
			new_block=(int)i;
			break;
		}
	}
	if (new_block==-1) { // if no free block, reply with error no space left
		fuse_reply_err(req,ENOSPC);
		return;
	}

	// find a free dentry in the parent directory
	int new_dentry=-1;
	for (size_t i=0;i<dentry_count;i++) {
		if (dentry_cache[i].inode==0) { // if the dentry is free, use it
			new_dentry=i;
			break;
		}
	}
	if (new_dentry==-1) { // if no free dentry, reply with error no space left
		fuse_reply_err(req,ENOSPC);
		return;
	}

	// create the new inode in the inode cache
	inode_cache[new_inode].inode=new_inode;
	inode_cache[new_inode].block_no=new_block;
	inode_cache[new_inode].size=0;
	inode_cache[new_inode].nlink=1;
	inode_cache[new_inode].mode=S_IFREG | (mode & 07777);
	inode_cache[new_inode].perm=mode & 07777;

	// set the user id and group id of the new inode to the current user and group
	inode_cache[new_inode].uid=getuid();
	inode_cache[new_inode].gid=getgid();

	sb->inode_bitmap|=(1<<new_inode); // update the inode bitmap to mark the new inode as used
	sb->block_bitmap|=(1<<new_block); // update the block bitmap to mark the new block as used
	dentry_cache[new_dentry].inode=new_inode; // update the dentry to mark it as used
	strncpy(dentry_cache[new_dentry].name,name,TOSFS_MAX_NAME_LENGTH); // copy the name of the new file to the dentry

	// reply with the new inode information
	struct fuse_entry_param e;
	memset(&e,0,sizeof(e));
	e.ino=new_inode;
	e.attr_timeout=1.0;
	e.entry_timeout=1.0;
	if (hello_stat(e.ino,&e.attr)==-1) { // check if the new inode number is valid
		fuse_reply_err(req,ENOENT);
		return;
	}
	fuse_reply_create(req,&e,fi);
	// print the creation of the new file
	printf("Created new file: %s (inode %d, block %d) in directory inode %lu\n",name,new_inode,new_block,parent);
}


// function to write data in a file in the filesystem given its inode number, the data to write, the size of the data and the offset
static void hello_ll_write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
	(void) fi;

	// check if the inode number is valid (exists and is a file)
	if (ino==0 || ino>sb->inodes || !((sb->inode_bitmap>>ino)&1) || (inode_cache[ino].mode & S_IFMT)!=S_IFREG) {
		fuse_reply_err(req,ENOENT);
		return;
	}

	// check if the offset is valid (not negative and not bigger than the block size)
	if (off<0 || off>=TOSFS_BLOCK_SIZE) {
		fuse_reply_err(req,EINVAL); // error invalid argument
		return;
	}

	// check if the size is valid (not negative and not bigger than the block size minus the offset)
	if (off > TOSFS_BLOCK_SIZE || size > TOSFS_BLOCK_SIZE - (size_t)off) {
		fuse_reply_err(req,EINVAL);
		return;
	}

	// write the data in the file
	char *data=(char *)fs_init + inode_cache[ino].block_no*TOSFS_BLOCK_SIZE; // get the pointer to the data of the file in the image
	memcpy(data+off,buf,size); // copy the data to the file at the given offset
	fuse_reply_write(req,size); // reply with the number of bytes written

	// update the size of the file in the inode cache
	if (off+size>inode_cache[ino].size) {
		inode_cache[ino].size=off+size;
	}
}
// to test the write function: echo "Hello World" > /path/to/mountpoint/newfile.txt


static struct fuse_lowlevel_ops hello_ll_oper = {
	.lookup		= hello_ll_lookup,
	.getattr	= hello_ll_getattr,
	.readdir	= hello_ll_readdir,
	.open		= hello_ll_open,
	.read		= hello_ll_read,
	.setattr	= hello_ll_setattr,
	.create = hello_ll_create,
	.write = hello_ll_write,
};

static int load_fs()
{
	// open the file
	int fd=open("test_tosfs_files.test", O_RDWR);
	if (fd<0) {
		perror("open");
		return -1;
	}

	// struct stat to get the size of the file 
	struct stat st;
	if (fstat(fd,&st)<0) {
		perror("fstat");
		close(fd);
		return -1;
	}
	fs_size=st.st_size; 

	// mmap the file
	fs_init=mmap(NULL,fs_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if (fs_init==MAP_FAILED) {
		perror("mmap");
		close(fd);
		return -1;
	}
	close(fd);

	// get the superblock and check the magic number
	sb=(struct tosfs_superblock *)fs_init;
	if (sb->magic!=TOSFS_MAGIC) {
		fprintf(stderr,"Invalid magic number: 0x%x\n",sb->magic);
		return -1;
	}

	// set bigger number of inodes available in the superblock to test the creation of new files
	sb->inodes=6;

	// get the inode cache
	inode_cache=(struct tosfs_inode *)((char *)fs_init + TOSFS_INODE_BLOCK*TOSFS_BLOCK_SIZE);

	// update the block bitmap to mark the blocks used by the inodes as used 
	for (unsigned i = 1; i <= sb->inodes; i++) {
		if (((sb->inode_bitmap >> i) & 1) && inode_cache[i].block_no < sb->blocks) // check if the inode is used and the block number is valid
			sb->block_bitmap |= 1u << inode_cache[i].block_no; // set the bit corresponding to the block number of the inode in the block bitmap
	}
	return 0;
}

int main(int argc, char *argv[])
{
	// load the filesystem image
	if (load_fs()<0) { 
		fprintf(stderr,"Failed to load the filesystem image\n");
		return 1;
	}

	printf("filesystem image loaded: %zu bytes, %u blocks, %u inodes\n",fs_size,sb->blocks,sb->inodes);
	
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_chan *ch;
	char *mountpoint;
	int err = -1;

	if (fuse_parse_cmdline(&args, &mountpoint, NULL, NULL) != -1 &&
	    (ch = fuse_mount(mountpoint, &args)) != NULL) {
		struct fuse_session *se;

		se = fuse_lowlevel_new(&args, &hello_ll_oper,
				       sizeof(hello_ll_oper), NULL);
		if (se != NULL) {
			if (fuse_set_signal_handlers(se) != -1) {
				fuse_session_add_chan(se, ch);
				err = fuse_session_loop(se);
				fuse_remove_signal_handlers(se);
				fuse_session_remove_chan(ch);
			}
			fuse_session_destroy(se);
		}
		fuse_unmount(mountpoint, ch);
	}
	fuse_opt_free_args(&args);

	return err ? 1 : 0;
}
