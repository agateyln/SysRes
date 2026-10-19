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

static const char *hello_str = "Hello World!\n";
static const char *hello_name = "hello";

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

static void hello_ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;

	if (parent != 1 || strcmp(name, hello_name) != 0)
		fuse_reply_err(req, ENOENT);
	else {
		memset(&e, 0, sizeof(e));
		e.ino = 2; 
		e.attr_timeout = 1.0;
		e.entry_timeout = 1.0;
		hello_stat(e.ino, &e.attr);

		fuse_reply_entry(req, &e);
	}
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
	if (off < bufsize)
		return fuse_reply_buf(req, buf + off,
				      min(bufsize - off, maxsize));
	else
		return fuse_reply_buf(req, NULL, 0);
}

static void hello_ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
			     off_t off, struct fuse_file_info *fi)
{
	(void) fi;

	if (ino != 1)
		fuse_reply_err(req, ENOTDIR);
	else {
		struct dirbuf b;

		memset(&b, 0, sizeof(b));
		dirbuf_add(req, &b, ".", 1);
		dirbuf_add(req, &b, "..", 1);
		dirbuf_add(req, &b, hello_name, 2);
		reply_buf_limited(req, b.p, b.size, off, size);
		free(b.p);
	}
}

static void hello_ll_open(fuse_req_t req, fuse_ino_t ino,
			  struct fuse_file_info *fi)
{
	if (ino != 2)
		fuse_reply_err(req, EISDIR);
	else if ((fi->flags & 3) != O_RDONLY)
		fuse_reply_err(req, EACCES);
	else
		fuse_reply_open(req, fi);
}

static void hello_ll_read(fuse_req_t req, fuse_ino_t ino, size_t size,
			  off_t off, struct fuse_file_info *fi)
{
	(void) fi;

	assert(ino == 2);
	reply_buf_limited(req, hello_str, strlen(hello_str), off, size);
}

static struct fuse_lowlevel_ops hello_ll_oper = {
	.lookup		= hello_ll_lookup,
	.getattr	= hello_ll_getattr,
	.readdir	= hello_ll_readdir,
	.open		= hello_ll_open,
	.read		= hello_ll_read,
};


static int load_fs()
{
	// open the file
	int fd=open("test_tosfs_files",O_RDWR);
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

	// get the inode cache
	inode_cache=(struct tosfs_inode *)((char *)fs_init + TOSFS_INODE_BLOCK*TOSFS_BLOCK_SIZE);
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
