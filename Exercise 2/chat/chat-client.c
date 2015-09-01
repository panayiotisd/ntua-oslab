/*
 * chat-client.c
 * Simple TCP/IP communication using sockets
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 */

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
#include <crypto/cryptodev.h>

#include "socket-common.h"


/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;
	
	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}

int main(int argc, char *argv[])
{
	int sd, port;
	ssize_t n;
	unsigned char buf[BUF_SIZE];
	unsigned char newbuf[BUF_SIZE];
	char *hostname;
	struct hostent *hp;
	struct sockaddr_in sa;
	
	memset(buf, 0, sizeof(buf));
	
	if (argc != 4) {
		fprintf(stderr, "Usage: %s hostname port key\n", argv[0]);
		exit(1);
	}
	hostname = argv[1];
	port = atoi(argv[2]); /* Needs better error checking */
	unsigned char key[KEY_SIZE];
	memcpy(key, argv[3], 16);
	
	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");
	
	/* Look up remote hostname on DNS */
	if ( !(hp = gethostbyname(hostname))) {
		printf("DNS lookup failed for host %s\n", hostname);
		exit(1);
	}

	/* Connect to remote TCP port */
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
	fprintf(stderr, "Connecting to remote host... "); fflush(stderr);
	if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("connect");
		exit(1);
	}
	fprintf(stderr, "Connected.\n");
	
	/* Initialize crypto device */
	int cfd;

	cfd = open("/dev/crypto", O_RDWR);
	if (cfd < 0) {
		perror("open(/dev/crypto)");
		exit(1);
	}
	
	struct session_op sess;
	struct crypt_op cryp;

	memset(&sess, 0, sizeof(sess));
	memset(&cryp, 0, sizeof(cryp));
	
	/*
	 * Use random values for the encryption key,
	 * the initialization vector (IV), and the
	 * data to be encrypted
	 */
	
	unsigned char iv[BLOCK_SIZE]="asdfghjklzxcvbnm";
	
	cryp.iv = iv;

	sess.key = key;
	
	/*
	 * Get crypto session for AES128
	 */
	sess.cipher = CRYPTO_AES_CBC;
	sess.keylen = KEY_SIZE;

	if (ioctl(cfd, CIOCGSESSION, &sess)) {
		perror("ioctl(CIOCGSESSION)");
		exit(1);
	}
	
	cryp.ses = sess.ses;
	cryp.len = BUF_SIZE;

	/* Start chating... */
	for (;;) {
			struct timeval timeout;
			timeout.tv_sec  = sec;
			timeout.tv_usec = msec;
			
			fd_set set;
			FD_ZERO(&set);
			FD_SET(sd, &set);
			FD_SET(STDIN_FILENO, &set);

			int returned = select(sd + 1, &set, NULL, NULL, &timeout);

			if ( returned > 0 ){
				if (FD_ISSET(sd, &set)) {
					n = read(sd, buf, sizeof(buf));
					if (n <= 0) {
						if (n < 0)
							perror("read from remote peer failed");
						else
							fprintf(stderr, "Server went away\n");
						break;
					}
					
					/*
					 * Decrypt data
					 */
					cryp.src = buf;
					cryp.dst = newbuf;
					cryp.len = BUF_SIZE;
					cryp.op = COP_DECRYPT;
					if (ioctl(cfd, CIOCCRYPT, &cryp)) {
						perror("ioctl(CIOCCRYPT)");
						exit(1);
					}		
					
					printf("Server: %s", newbuf);
				}
				if (FD_ISSET(STDIN_FILENO, &set)) {
					n = read(STDIN_FILENO, buf, sizeof(buf));
					if ( !memcmp("exit\n", buf, 5) ) {
						break;	//shutdown						
					}
					/*
					 * Encrypt data.in to data.encrypted
					 */
					cryp.ses = sess.ses;
					cryp.src = buf;
					cryp.dst = newbuf;
					cryp.len = BUF_SIZE;
					//change iv////////////////////////////////
					cryp.op = COP_ENCRYPT;

					if (ioctl(cfd, CIOCCRYPT, &cryp)) {
						perror("ioctl(CIOCCRYPT)");
						exit(1);
					}
					
					if (insist_write(sd, newbuf, BUF_SIZE) != BUF_SIZE) {
						perror("write to remote peer failed");
						break;
					}
				}
			}
			else if ( returned < 0 ) {
				perror("select failed!");
				exit(1);
			}

			memset(buf, 0, sizeof(buf));
	}
	
	

	/*
	 * Let the remote know we're not going to write anything else.
	 * Try removing the shutdown() call and see what happens.
	 */
	if (shutdown(sd, SHUT_WR) < 0) {
		perror("shutdown");
		exit(1);
	}

	/* Read answer and write it to standard output */
	for (;;) {
		n = read(sd, buf, sizeof(buf));

		if (n < 0) {
			perror("read");
			exit(1);
		}

		if (n <= 0)
			break;

		if (insist_write(0, buf, n) != n) {
			perror("write");
			exit(1);
		}
	}
	
	/* Finish crypto session */
	if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
		perror("ioctl(CIOCFSESSION)");
		return 1;
	}
	
	if (close(cfd) < 0) {
		perror("close(fd)");
		return 1;
	}

	fprintf(stderr, "\nDone.\n");
	return 0;
}
