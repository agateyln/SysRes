#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

int data=5;
int bss=0;
char str[]="Hello";

int main() {
    pid_t pid = getpid();
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

    int child = fork();
    if (child == -1) {
        perror("fork");
        free(heap);
        munmap(mapped, sizeof(int));
        return 1;
    }
    if (child == 0) {
        char pid_string[32];
        printf("Child process\n");
        snprintf(pid_string, sizeof(pid_string), "%ld", (long)pid);
        execlp("/usr/bin/pmap", "pmap", "-X", pid_string, (char *)NULL);
        perror("execl");
    } else {
        printf("Parent process\n");
        waitpid(child, NULL, 0);
    }
    
    fflush(stdout);
    free(heap);
    munmap(mapped, 4096);
    return 0;
}