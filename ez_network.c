#include "ez_network.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
/**
 * Attempts to bind an address to a socket.
 * param: sock (int)         -> a socket file descriptor
 * param: family (int)       -> an address family
 * param: address (int)      -> an address
 * param: port (int*)  [out] -> the port. if 0, port will be randomly selected & this parameter will be set to its value on return.
   return: (int) 	     -> 1 on success, 0 on failure.
**/
int ez_bind(int sock, int family, long address, int* port) {
	struct sockaddr_in addr_info;
	memset(&addr_info, 0, sizeof(struct sockaddr_in));
        addr_info.sin_family        = family;               // todo: support for ipv6
        addr_info.sin_port          = htons(*port);              // give us a random port
        addr_info.sin_addr.s_addr   = htonl(address); // use any addr
        // todo: more robust error handling for binding
        if(bind(sock, (struct sockaddr*) &addr_info, sizeof(struct sockaddr_in)) == -1) {
                printf("Error: failed to bind rtp to port. Errno:%d\n", errno);
                return 0;
        }
        socklen_t len = sizeof(addr_info);
        if(getsockname(sock, (struct sockaddr*) &addr_info, &len) == -1) {
                printf("Error: failed to get sock info. Errno:%d\n", errno);
        	return 0;
	}
	*port = ntohs(addr_info.sin_port);
	return 1;
}

/**
 * Sends a packet to a host on the specified port.
 * param: sock (int)	  -> a socket file descriptor
 * param: data (void*)    -> a buffer containing the data to send
 * param: length (size_t) -> the length of the data buffer
 * param: addr_family (int) -> the address family AF_INET | AF_INET6
 * param: host (char*)    -> the ipv4 address to send to in dot notation
 * param: port (int)	  -> the port to send to
 * return: (int)	  -> 1 on success, 0 on failure
**/
int ez_sendto(int sock, void* data, size_t length, int addr_family, char* host, int port) {
	struct sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(struct sockaddr_in));
	dest_addr.sin_family = addr_family;
	dest_addr.sin_port   = htons(port);
	if(inet_pton(addr_family, host, &(dest_addr.sin_addr)) == 0) {
		printf("Error: invalid IP address:%s\n", host);
		return 0;
	}
	if(sendto(sock, data, length, 0, (struct sockaddr*) &dest_addr, sizeof(struct sockaddr_in)) != length) {
		return 0;
	}
	return 1;
}
/**
 * Attempts to read a datagram from a UDP socket. Note: this function does NOT block.
 * param: sock (int)      -> a socket file descriptor
 * param: buf (void*)     -> a region of memory where the datagram will be stored
 * param: length (size_t) -> the length of the supplied buffer
 * return: number of bytes read on success, -1 on failure
**/
int ez_recv_noblock(int sock, void* buf, size_t length) {
	size_t bytes_recvd = recvfrom(sock, buf, length, MSG_DONTWAIT, NULL, NULL);
	
	return bytes_recvd;
}
