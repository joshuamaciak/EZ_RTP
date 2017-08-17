#include "ez_rtp.h"
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
int urandom(uint32_t* r);
/**
 * Initializes an rtp session. On success rtp_session will be populated with necessary values.
 * param: (struct rtp_session*) -> A pointer to an empty rtp_session.
 * return: (int)                -> 1 on success, 0 on failure 
**/
int rtp_session_init(struct rtp_session* session) {
	memset(session, 0, sizeof(struct rtp_session));
	uint32_t rand_ssrc;
	if(urandom(&rand_ssrc) == 0) {
		return 0;
	}
	session->ssrc = rand_ssrc;
	session->rtp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(session->rtp_sock == -1) {
		printf("Error: failed to create rtp socket. Errno%d\n", errno);
		return 0;
	}
	session->rtcp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(session->rtcp_sock == -1) {
                printf("Error: failed to create rtp socket. Errno:%d\n", errno);
                return 0;
        }
	session->rtp_port = 0;
	if(ez_bind(session->rtp_sock, AF_INET, INADDR_ANY, &(session->rtp_port)) == 0) {
		return 0;
	}

	// try to get an rtcp port next to rtp
	int rtcp_port = session->rtp_port + 1;
	if(rtcp_port > 65535) { // since port is 2 octets, 65535 is max 
		rtcp_port -= 2;
	}
        if(ez_bind(session->rtcp_sock, AF_INET, INADDR_ANY, &rtcp_port) == 0) {
                return 0;
        }
	session->rtcp_port = rtcp_port;
	return 1;	
}
/**
 * Gets a 4 octet random number from /dev/urandom (apparently it is the most * secure random source. 
 * param: (uint32_t*) -> A pointer that points to storage 
 * return: (int)      -> 1 on success, 0 on failure
**/
int urandom(uint32_t* r) {
	int urand_fd = open("/dev/urandom", O_RDONLY);
	if(urand_fd == 0) {
		printf("Critical error: couldn't open /dev/urandom.\n");
		return 0;
	}
	read(urand_fd, &r, sizeof(uint32_t));
	close(urand_fd);
	return 1; 
}
