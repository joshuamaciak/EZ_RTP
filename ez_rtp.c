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
	// initialize participant info list
	session->participants = calloc(1, sizeof(struct participant_info));
	return 1;	
}
/** 
 * Blocks until an RTP packet is received.
 * param: session (struct rtp_session*) -> an active rtp session
 * param: packet (struct rtp_packet**)	-> the region that will store the received rtp_packet
 * param: packet_length (size_t*) 	-> will contain the size of the received rtp_packet on return
 * return: (int)			-> 0 on failure, 1 on success
**/
int rtp_recv(struct rtp_session* session, struct rtp_packet** packet, size_t* packet_length) {
	int res = 0;
	*packet_length = MAX_DATAGRAM_SIZE;
	*packet = malloc(MAX_DATAGRAM_SIZE);
	res = ez_recv_noblock(session->rtp_sock, *packet, *packet_length);
	if(res > 0) {
		*packet_length = res;
	} else {
		*packet_length = 0;
	
	} 
	
	if(res == -1 && (errno != EAGAIN && errno != EWOULDBLOCK)) return 0;
	
	// todo: add participants if not in list
	return 1;	
	
}
/**
 * Adds a participant to the RTP session participant list
 * param: (rtp_session*) session -> an active RTP session
 * param: (uint32_t) ssrc	 -> the ssrc of the participant
**/
void add_participant(struct rtp_session* session, uint32_t ssrc) {
	session->num_participants++;
	session->participants = realloc(session->participants, sizeof(struct participant_info) * session->num_participants);
	session->participants[session->num_participants - 1].ssrc = ssrc;
}
/**
 * Removes a participant from the RTP session participant list
 * param: (rtp_session*) session -> an active RTP session
 * param: (uint32_t) ssrc	 -> the ssrc of the participant
 * return: (int) 		 -> 1 if remove was successful, 0 if ssrc wasn't found
**/
int remove_participant(struct rtp_session* session, uint32_t ssrc) {
	int index_for_removal = find_participant(session, ssrc);
	// if found, shift all remaining elements down
	if(index_for_removal == -1) {
		return 0;
	}
	for(int i = index_for_removal + 1; i < session->num_participants; ++i) {
		session->participants[i - 1] = session->participants[i]; 	
	}
	session->num_participants--;
	return 1;
}
/**
 * Finds a participant in the RTP session participant list via ssrc
 * param: (rtp_session*) session -> an active RTP session
 * param: (uint32_t) ssrc	 -> the ssrc of the participant
 * return: (int) 		 -> the index of the participant if found, otherwise -1
**/
int find_participant(struct rtp_session* session, uint32_t ssrc) {
	for(int i = 0; i < session->num_participants; ++i) {
		if(session->participants[i].ssrc == ssrc) return i;
	}
	return -1;
}
/**
 * Prints a list of all the current participants in an RTP session
 * param: (rtp_session*) -> an active RTP sessioni
**/
void print_participants(struct rtp_session* session) {
	printf("Printing %d participants:\n", session->num_participants);
	for(int i = 0; i < session->num_participants; ++i) {
		printf("Participant %u\n", session->participants[i].ssrc);
	}
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
 * Calculates the size of an RTP header. 
 * param: (rtp_header*) header -> the rtp header
 * return: (size_t) the size of the RTP header.
 *
 * (impl. note): we cannot simply assume a header is 16 octets b/c 
 * variable length list of csrc's and possible header extension.
 * Smallest possible rtp_header is 12 octets.
**/
size_t rtp_header_size(struct rtp_header* header) {
	size_t size = sizeof(struct rtp_header);
	int csrc_count = GET_CSRC_COUNT(header->bitfields);
	size += (csrc_count - 1) * sizeof(uint32_t);
	// todo: need to add support for header extensions
	return size;	
}
/**
 * Calculates the size of the payload within an RTP packet
 * param: (rtp_packet*) packet -> the rtp packet
 * param: (size_t) packet_size -> the size of the total packet
 * return: (size_t) the size of the payload.
**/
size_t rtp_payload_size(struct rtp_packet* packet, size_t packet_size) {
	size_t payload_size = packet_size - rtp_header_size(&(packet->header));
	// todo: need to add support for packet padding
	return payload_size;
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
