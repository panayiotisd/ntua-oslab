/*
 * chat-server.c
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
	unsigned char buf[BUF_SIZE];
	unsigned char newbuf[BUF_SIZE];
	char addrstr[INET_ADDRSTRLEN];
	int sd, newsd;
	ssize_t n;
	socklen_t len;
	struct sockaddr_in sa;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s key\n", argv[0]);
		exit(1);
	}
	unsigned char key[KEY_SIZE];
	memcpy(key, argv[1], 16);
	
	memset(buf, 0, sizeof(buf));
	
	/* Make sure a broken connection doesn't kill us */
	signal(SIGPIPE, SIG_IGN);

	/* Create TCP/IP socket, used as main chat channel */
	if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");

	/* Bind to a well-known port */
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(TCP_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		perror("bind");
		exit(1);
	}
	fprintf(stderr, "Bound TCP socket to port %d\n", TCP_PORT);

	/* Listen for incoming connections */
	if (listen(sd, TCP_BACKLOG) < 0) {
		perror("listen");
		exit(1);
	}

	/* Loop forever, accept()ing connections */
	for (;;) {
		fprintf(stderr, "Waiting for an incoming connection...\n");

		/* Accept an incoming connection */
		len = sizeof(struct sockaddr_in);
		if ((newsd = accept(sd, (struct sockaddr *)&sa, &len)) < 0) {
			perror("accept");
			exit(1);
		}
		if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr))) {
			perror("could not format IP address");
			exit(1);
		}
		fprintf(stderr, "Incoming connection from %s:%d\n",
			addrstr, ntohs(sa.sin_port));
		
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
			FD_SET(newsd, &set);
			FD_SET(STDIN_FILENO, &set);

			int returned = select(newsd + 1, &set, NULL, NULL, &timeout);

			if ( returned > 0 ){
				if (FD_ISSET(newsd, &set)) {
					n = read(newsd, buf, sizeof(buf));
					if (n <= 0) {
						if (n < 0)
							perror("read from remote peer failed");
						else
							fprintf(stderr, "Peer went away\n");
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
					
					printf("Client: %s", newbuf);
				}
				if (FD_ISSET(STDIN_FILENO, &set)) {
					n = read(STDIN_FILENO, buf, sizeof(buf));
					
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
					
					if (insist_write(newsd, newbuf, BUF_SIZE) != BUF_SIZE) {
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
		/* Make sure we don't leak open files */
		if (close(newsd) < 0)
			perror("close");
			
		/* Finish crypto session */
		if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
			perror("ioctl(CIOCFSESSION)");
			return 1;
		}
		
		if (close(cfd) < 0) {
			perror("close(fd)");
			return 1;
		}
	}

	/* This will never happen */
	return 1;
}

