#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#define FILE_LENGTH 0x100

int main (int argc, char* const argv[])
{
	int fd;
	void* file_memory;
	
	/* Prepare a file large enough to hold an unsigned integer. */
	fd = open (".hello_there", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	ftruncate(fd, 32768);

	close(fd);
	return 0;
}
