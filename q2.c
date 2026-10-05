#include <stdio.h>
#include <unistd.h>

int main(void)
{
	const char *filename = "test.txt";
	FILE *file = fopen(filename, "w");

	if (file == NULL)
		return 1;

	execl("/usr/bin/stat", "stat", filename, (char *)NULL);
	perror("execl");
	fclose(file);
	return 1;
}

