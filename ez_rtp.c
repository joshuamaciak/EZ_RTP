#include "ez_rtp.h"
#include "ez_network.h"
#include <sys/types.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
int urandom32(uint32_t* r);
int urandom16(uint16_t* r);
/**
 * Initializes an rtp session. On success rtp_session will be populated with necessary values.
 * param: (struct rtp_session*) -> A pointer to an empty rtp_session.
 * return: (int)                -> 1 on success, 0 on failure 
**/
int rtp_session_init(struct rtp_session* session) {
	memset(session, 0, sizeof(struct rtp_session));
	uint32_t rand_ssrc;
	if(urandom32(&rand_ssrc) == 0) {
		return 0;
	}
        uint16_t rand_seq;
        if(urandom16(&rand_seq) == 0) {
                return 0;
        }
        uint32_t rand_timestamp;
        if(urandom32(&rand_timestamp) == 0) {
                return 0;
        }	
	
	session->ssrc = rand_ssrc;
	session->init_seq  = rand_seq;
	session->last_seq  = session->init_seq - 1;
	session->init_timestamp = rand_timestamp;
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
 * Sends an RTP packet to all of the participants.
 * param: session (struct rtp_session*)      -> an active rtp_session
 * param: packet  (struct rtp_packet*)       -> an rtp packet
 * param: packet_length (struct rtp_packet*) -> the length of the packet (header + ext + payload + padding + anything else) in octets
 * return: (int)                             -> 1 on success, 0 on failure
**/
int rtp_send(struct rtp_session* session, struct rtp_packet* packet, size_t packet_length) {
	int success = 0;
	for(int i = 0; i < session->num_participants; ++i) {	
		int res = ez_sendto(session->rtp_sock, (void*) packet, packet_length, AF_INET, session->participants[i].host, session->participants[i].rtp_port);
		if(res == 0) {
			printf("Failed to send to host:%s\n", session->participants[i].host);
		}		
		success += res;
	}

	return (success == session->num_participants) ? 1 : 0;
}
/**
 * Gets a 4 octet random number from /dev/urandom (apparently it is the most * secure random source. 
 * param: (uint32_t*) -> A pointer that points to storage 
 * return: (int)      -> 1 on success, 0 on failure
**/
int urandom32(uint32_t* r) {
	int urand_fd = open("/dev/urandom", O_RDONLY);
	if(urand_fd == 0) {
		printf("Critical error: couldn't open /dev/urandom.\n");
		return 0;
	}
	read(urand_fd, r, sizeof(uint32_t));
	close(urand_fd);
	return 1; 
}
int urandom16(uint16_t* r) {
        int urand_fd = open("/dev/urandom", O_RDONLY);
        if(urand_fd == 0) {
                printf("Critical error: couldn't open /dev/urandom.\n");
                return 0;
        }
        read(urand_fd, r, sizeof(uint16_t));
        close(urand_fd);
        return 1;
}
