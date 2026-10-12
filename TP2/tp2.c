#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include<stdint.h>

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next; /* pointe sur le prochain bloc libre */
    size_t bloc_size; /* taille du memory bloc en octets*/
    long magic_number; /* 0xBADA110CBADA110CL */
} HEADER;

#define MAGIC_NUMBER 0xBADA110CBADA110CL
#define HEADER_SIZE sizeof(HEADER)
#define TAIL_SIZE sizeof(long)

HEADER * free_list = NULL; /* pointeur sur le premier bloc libre */

void *malloc_3is (size_t size){
    if (size == 0) {return NULL;}
    size_t total_size = size + HEADER_SIZE + TAIL_SIZE;
    HEADER *p= sbrk(total_size);
    p->ptr_next=NULL;
    p->bloc_size=size;
    p->magic_number=MAGIC_NUMBER;
    return (char *)p + HEADER_SIZE;
}

typedef struct free_block {
    int data;
    struct free_block *next;
} free_block;

free_block  *free_list_head=NULL;

void free_3is(void *ptr) {
    if (ptr==NULL) {return;} 
    HEADER *header = (HEADER *)((char *)ptr - HEADER_SIZE); // get the header of the block
    free_block *new_block = (free_block *)header; // create a new free block from the header
    new_block->data=header->bloc_size; // store the size of the block in the data field
    new_block->next=NULL; 
    if (free_list_head==NULL || new_block<free_list_head) { // if the free list is empty or the new block's address is less than the head of the free list
        new_block->next=free_list_head; // add the new block to the front of the free list because it is the smallest address
        free_list_head=new_block; // update the head of the free list to point to the new block that was just freed
    } else { 
        free_block *current=free_list_head; // start at the head of the free list
        while (current->next!=NULL && current->next<new_block) { // going through the free list until we find the right place to insert the new block
            current=current->next; 
        }
        new_block->next=current->next;
        current->next=new_block; 
    }
}


int main(void){
    size_t size = 10;
    void *before = sbrk(0);
    void *block = malloc_3is(size);
    void *after = sbrk(0);

    printf("before: %p\n", before);
    printf("block:  %p\n", block);
    printf("after:  %p\n", after);

    printf("heap increase: %zu bytes\n",
           (size_t)((char *)after - (char *)before));

    printf("expected: %zu bytes\n",
           size + HEADER_SIZE + TAIL_SIZE);

    // test free_3is
    printf("freeing block: %p\n", block);
    printf("block size: %zu\n", size);
    free_3is(block);
    // check if the block is added to the free list
    if (free_list_head != NULL) {
        printf("block added to free list\n");
        printf("free list head: %p\n", free_list_head); 
        printf("free list head size: %d\n", free_list_head->data);
    }

    return 0;
}
