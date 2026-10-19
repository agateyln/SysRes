#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include "tosfs.h"
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    // open the file
    int fd=open("test_tosfs_files.test",O_RDWR); 
    if (fd<0) { 
        perror("open");
        return 1;
    }

    // struct stat to get the size of the file
    struct stat st;
    if (fstat(fd,&st)<0) {
        perror("fstat"); 
        return 1;
    }

    // print the size of the file and the number of blocks
    printf("file size: %ld octets\n", (long)st.st_size);
    printf("%ld blocks of %d octets\n", (long)st.st_size / TOSFS_BLOCK_SIZE, TOSFS_BLOCK_SIZE);

    // mmap the file 
    void *map=mmap(NULL,st.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    if (map==MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // get the superblock to print its content
    struct tosfs_superblock *sb=(struct tosfs_superblock *)map;

    printf("magic number: 0x%x\n",sb->magic);
    printf("block_size: %u\n",sb->block_size);
    printf("blocks: %u\n",sb->blocks);
    printf("inodes: %u\n",sb->inodes);
    printf("root_inode: %u\n",sb->root_inode);
    printf("block_bitmap: " PRINTF_BINARY_PATTERN_INT32 "\n", PRINTF_BYTE_TO_BINARY_INT32(sb->block_bitmap));
    printf("inode_bitmap: " PRINTF_BINARY_PATTERN_INT32 "\n", PRINTF_BYTE_TO_BINARY_INT32(sb->inode_bitmap));

    // print the content of the inode cache
    inode_cache=(struct tosfs_inode *)((char *)map + TOSFS_INODE_BLOCK*TOSFS_BLOCK_SIZE); 
    for (unsigned i=0; i<=sb->inodes;i++) {
        printf("index %u: inode=%u block_no=%u mode=%o perm=%o size=%u nlink=%u uid=%u gid=%u\n",
        i, inode_cache[i].inode, inode_cache[i].block_no,
        inode_cache[i].mode, inode_cache[i].perm, inode_cache[i].size,
        inode_cache[i].nlink, inode_cache[i].uid, inode_cache[i].gid);
    }

    // print the content of the dentry cache
    struct tosfs_dentry *dentry_cache=(struct tosfs_dentry *)((char *)map + inode_cache[sb->root_inode].block_no * TOSFS_BLOCK_SIZE);
    for (unsigned i=0;i<sb->inodes+1;i++) {
        printf("dentry %u: inode=%u name=%.*s\n",i,dentry_cache[i].inode,TOSFS_MAX_NAME_LENGTH,dentry_cache[i].name);
    }

    for (unsigned n=0;n<=sb->inodes;n++) {
        char *data=(char *)map + inode_cache[n].block_no*TOSFS_BLOCK_SIZE;
        printf("content of inode %u: (%u bytes): %.*s\n",n,inode_cache[n].size,(int)inode_cache[n].size,data);
    }


    munmap(map, st.st_size);
    close(fd);
    return 0; 
}
