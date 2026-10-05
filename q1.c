#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int data=5;
int bss=0;
char str[]="Hello";

int main() {
    int local=10;
    void *heap=malloc(sizeof(int));
    void *mapped=mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    printf("Address of data: %p\n", (void*)&data);
    printf("Address of bss: %p\n", (void*)&bss);
    printf("Address of str: %p\n", (void*)str);
    printf("Address of local: %p\n", (void*)&local);
    printf("Address of mapped: %p\n", (void*)mapped);
    printf("Address of heap: %p\n", (void*)heap);
    printf("Address of function in shared library: %p\n", (void*)&printf);
    printf("Address of main: %p\n", (void*)&main);

    char command[64];
    snprintf(command, sizeof(command), "pmap -X %ld", (long)getpid());
    system(command);
    free(heap);
    return 0;
}