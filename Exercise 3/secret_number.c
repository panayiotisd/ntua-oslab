#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{
	ssize_t n;
	char buf[500];
	int cfd;

	cfd = open("secret_number", O_RDWR, 0);
	if (cfd < 0) {
		perror("open");
		exit(1);
	}
	
	struct timeval timeout;
	timeout.tv_sec  = 5;
	timeout.tv_usec = 0;
			
	fd_set set;
	FD_ZERO(&set);
	FD_SET(cfd, &set);
while(1)  {

	int returned = select(cfd + 1, &set, NULL, NULL, &timeout);

	if ( returned > 0 ){
		n = read(cfd, buf, sizeof(buf));
		printf("%s", buf);
	}

}
	return 0;
}
