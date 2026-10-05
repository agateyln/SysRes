#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void)
{
	const char *filename = "test.txt";
	FILE *file = fopen(filename, "w"); 
	struct stat file_stat;
	stat(filename,&file_stat);
	size_t size = file_stat.st_size;

	printf("Size of %s: %zu bytes\n", filename, size);
	//if (file == NULL)
	//	return 1;

	//execl("/usr/bin/stat", "stat", filename, (char *)NULL);
	//perror("execl");

	fclose(file);
	return 1;
}

