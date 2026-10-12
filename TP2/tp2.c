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
    return (char *)p + HEADER_SIZE;
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


    return 0;
}
