/*
 * socket-common.h
 *
 * Simple TCP/IP communication using sockets
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 */

#ifndef _SOCKET_COMMON_H
#define _SOCKET_COMMON_H

/* Compile-time options */
#define TCP_PORT    35001
#define TCP_BACKLOG 5

#define BUF_SIZE       256
#define BLOCK_SIZE      16
#define KEY_SIZE	16  /* AES128 */

#define sec 0
#define msec 100

#endif /* _SOCKET_COMMON_H */

