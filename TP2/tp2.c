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

    if (current && (char *)block + HEADER_SIZE + block->bloc_size + TAIL_SIZE == (char *)current) {
        block->bloc_size += HEADER_SIZE + TAIL_SIZE + current->bloc_size;
        block->ptr_next = current->ptr_next;
        *(unsigned long *)((char *)block + HEADER_SIZE + block->bloc_size) = MAGIC_NUMBER;
    }

    if (previous && (char *)previous + HEADER_SIZE + previous->bloc_size + TAIL_SIZE == (char *)block) {
        previous->bloc_size += HEADER_SIZE + TAIL_SIZE + block->bloc_size;
        previous->ptr_next = block->ptr_next;
        *(unsigned long *)((char *)previous + HEADER_SIZE + previous->bloc_size) = MAGIC_NUMBER;
    }
}


void preallocate(size_t size){
    if (size == 0 || free_list != NULL) {return;}

    size_t total_size = size + HEADER_SIZE + TAIL_SIZE;
    HEADER *p = sbrk(total_size);
    if (p == (void *)-1) {return;}

    p->ptr_next = NULL;
    p->bloc_size = size;
    p->magic_number = MAGIC_NUMBER;
    *(unsigned long *)((char *)p + HEADER_SIZE + size) = MAGIC_NUMBER;

    insert_free_block(p);
}



// Recherche un bloc libre et le découpe si le reliquat peut former un bloc libre.
void *check_free_block(size_t size) {
    
    HEADER *previous = NULL;
    HEADER *current = free_list;

    while (current) {
        if (current->bloc_size >= size) {
            if (previous) { 
                previous->ptr_next = current->ptr_next; 
            } else {
                free_list = current->ptr_next; 
            }

            if (current->bloc_size >= size + HEADER_SIZE + TAIL_SIZE + 1) {
                size_t remaining_size = current->bloc_size - size - HEADER_SIZE - TAIL_SIZE;
                HEADER *remaining = (HEADER *)((char *)current + HEADER_SIZE + size + TAIL_SIZE);
                remaining->ptr_next = NULL;
                remaining->bloc_size = remaining_size;
                remaining->magic_number = MAGIC_NUMBER;
                *(unsigned long *)((char *)remaining + HEADER_SIZE + remaining_size) = MAGIC_NUMBER;
                insert_free_block(remaining);
            }

            current->bloc_size = size;
            current->ptr_next = NULL;
            current->magic_number = MAGIC_NUMBER;
            *(unsigned long *)((char *)current + HEADER_SIZE + size) = MAGIC_NUMBER;
            return (char *)current + HEADER_SIZE;
        }
        previous = current;
        current = current->ptr_next;
    }
    return NULL; // No suitable block found
}

int check_magic_numbers(void *ptr){
    if (ptr == NULL) {return 0;}

    HEADER *block = (HEADER *)((char *)ptr - HEADER_SIZE);
    if (block->magic_number != MAGIC_NUMBER) {return 0;}
    unsigned long *tail = (unsigned long *)((char *)ptr + block->bloc_size);

    return *tail == MAGIC_NUMBER;
}

void *malloc_3is (size_t size){
    if (size == 0) {return NULL;}
    void *block = check_free_block(size);
    if (block) { // if a free block of the requested size is found
        HEADER *p = (HEADER *)((char *)block - HEADER_SIZE); // get the header of the block
        p->magic_number = MAGIC_NUMBER; 
        p->bloc_size = size;
        p->ptr_next = NULL;
        *(unsigned long *)((char *)block + size) = MAGIC_NUMBER; // set the magic number at the end of the block
        return block;
    } 
    else {
        size_t total_size = size + HEADER_SIZE + TAIL_SIZE;
        HEADER *p= sbrk(total_size);
        if (p == (void *)-1) {return NULL;}
        p->ptr_next = NULL;
        p->bloc_size = size;
        p->magic_number = MAGIC_NUMBER;
        *(unsigned long *)((char *)p + HEADER_SIZE + size) = MAGIC_NUMBER;
        return (char *)p + HEADER_SIZE;
    }
}



void free_3is(void *ptr){
    if (ptr == NULL) {return;}
    HEADER *block = (HEADER *)((char *)ptr - HEADER_SIZE);
    if (!check_magic_numbers(ptr)) {return;}
    insert_free_block(block);
}





int main(void){
    size_t size = 10;
    preallocate(256);
    printf("preallocated free block: %zu bytes\n", free_list ? free_list->bloc_size : 0);

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

    void *adjacent_first = malloc_3is(30);
    void *adjacent_second = malloc_3is(40);
    free_3is(adjacent_first);
    free_3is(adjacent_second);
    HEADER *merged = free_list;
    while (merged && merged->bloc_size != 30 + HEADER_SIZE + TAIL_SIZE + 40) {
        merged = merged->ptr_next;
    }
    printf("merged adjacent size: %zu bytes\n",
        merged ? merged->bloc_size : 0);

    void *second_block = malloc_3is(size);
    free_3is(second_block);
    printf("free list: %p -> %p\n",
        (void *)free_list, (void *)free_list->ptr_next);
    free_3is(block);
    printf("free list: %p -> %p\n",
        (void *)free_list, (void *)free_list->ptr_next);
    
    
    // test recycling of free blocks
    void *break_before = sbrk(0);
    void *recycled_block = malloc_3is(size);
    void *break_after = sbrk(0);
    printf("recycled block: %p\n", recycled_block);
    printf("heap increase after recycling: %zu bytes\n",
              (size_t)((char *)break_after - (char *)break_before));

    void *large_block = malloc_3is(100);
    free_3is(large_block);
    void *split_block = malloc_3is(20);
    HEADER *remaining = free_list;
    while (remaining && remaining->bloc_size != 100 - 20 - HEADER_SIZE - TAIL_SIZE) {
        remaining = remaining->ptr_next;
    }
    printf("split block: %p\n", split_block);
    printf("remaining free size: %zu bytes\n",
           remaining ? remaining->bloc_size : 0);
    return 0;
}
