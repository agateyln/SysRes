#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include<stdint.h>

typedef struct HEADER_TAG {
    struct HEADER_TAG * ptr_next; /* pointe sur le prochain bloc libre */
    size_t bloc_size; /* taille du memory bloc en octets*/
    unsigned long magic_number; /* 0xBADA110CBADA110C */
} HEADER;

#define MAGIC_NUMBER 0xBADA110CBADA110CL
#define HEADER_SIZE sizeof(HEADER)
#define TAIL_SIZE sizeof(unsigned long)

HEADER * free_list = NULL; /* pointeur sur le premier bloc libre */


void insert_free_block(HEADER *block){
    HEADER *previous = NULL;
    HEADER *current = free_list;

    while (current && current < block) { // Parcours de la liste pour trouver la position d'insertion
        previous = current;
        current = current->ptr_next;
    }
    if (current == block) {return;}

    block->ptr_next = current; 
    if (previous) {
        previous->ptr_next = block;
    } else {
        free_list = block;
    }
}

void *malloc_3is (size_t size){
    if (size == 0) {return NULL;}
    size_t total_size = size + HEADER_SIZE + TAIL_SIZE;
    HEADER *p= sbrk(total_size);
    if (p == (void *)-1) {return NULL;}
    p->ptr_next = NULL;
    p->bloc_size = size;
    p->magic_number = MAGIC_NUMBER;
    *(unsigned long *)((char *)p + HEADER_SIZE + size) = MAGIC_NUMBER;
    return (char *)p + HEADER_SIZE;
}

int check_magic_numbers(void *ptr){
    if (ptr == NULL) {return 0;}

    HEADER *block = (HEADER *)((char *)ptr - HEADER_SIZE);
    if (block->magic_number != MAGIC_NUMBER) {return 0;}
    unsigned long *tail = (unsigned long *)((char *)ptr + block->bloc_size);

    return *tail == MAGIC_NUMBER;
}

void free_3is(void *ptr){
    if (ptr == NULL) {return;}
    HEADER *block = (HEADER *)((char *)ptr - HEADER_SIZE);
    if (!check_magic_numbers(ptr)) {return;}
    insert_free_block(block);
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

    void *second_block = malloc_3is(size);
    free_3is(second_block);
    printf("free list: %p -> %p\n",
        (void *)free_list, (void *)free_list->ptr_next);
    free_3is(block);
    printf("free list: %p -> %p\n",
        (void *)free_list, (void *)free_list->ptr_next);
    
    

    return 0;
}
