#include "ez_network.h"
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

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
