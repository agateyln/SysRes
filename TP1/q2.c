#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>


int main(void)
{
	const char *filename = "test.txt";
	int file = open(filename, O_RDWR);
	if (file == -1)  //Many checks for each step using files
		return 1;

    struct stat file_stat;
	if (stat(filename, &file_stat) == -1) {
		close(file);
        return 1;
    }

    size_t filesize = file_stat.st_size;
    printf("File size: %zu bytes\n", filesize);
	if (filesize == 0) {
		close(file);
		return 0;
	}

	unsigned char *filemap = mmap(NULL, filesize, PROT_READ | PROT_WRITE,
								  MAP_SHARED, file, 0);
	if (filemap == MAP_FAILED) {
		close(file);
		return 1;
	}

	for (size_t left = 0, right = filesize - 1; left < right;
		 left++, right--) {
		unsigned char byte = filemap[left];
		filemap[left] = filemap[right];
		filemap[right] = byte;
	}
	msync(filemap, filesize, MS_SYNC);
	munmap(filemap, filesize);
	close(file);

	fflush(stdout);
    int child = fork();  // Fork a child process to execute the cat command
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        execl("/usr/bin/cat", "cat", filename, (char *)NULL);
        perror("execl");
    }
    else {
        waitpid(child, NULL, 0);
    }
	return 0;
}

