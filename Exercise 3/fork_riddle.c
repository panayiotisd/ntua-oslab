#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int main (int argc, char* const argv[])
{
	while(1) {
		pid_t p;
		p = fork();
		if (p==0) {
			return 0;
		}
		if ( p >= 32760 ) {
			return 0;
		}
	}
	return 0;
}
