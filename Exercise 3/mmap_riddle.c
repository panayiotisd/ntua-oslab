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
	fd = open (argv[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	lseek (fd, 0x6f, SEEK_SET);
	write (fd, argv[2], 1);
	/* Create the memory mapping. */
	//file_memory = mmap (0, FILE_LENGTH + 0x6f, PROT_WRITE, MAP_SHARED, fd, 0);
	//close (fd);
	
	/* Write a random integer to memory-mapped area. */
	//sprintf((char*) file_memory + 0x6f, "%c", atoi(argv[2]));
	
	/* Release the memory (unnecessary because the program exits). */
	//munmap (file_memory, FILE_LENGTH + 0x6f);
	return 0;
}
