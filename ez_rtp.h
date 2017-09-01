#ifndef EZ_RTP_H
#define EZ_RTP_H
/**
 *				Real-time Transport Protocol
 * Author: Joshua Maciak
 * Date: August 13 2017
 *
 * The purpose of this library is to provide an implementation of the 
 * Real-time Transport Protocol (RTP) & the Real-time Transport Control 
 * Protocol (RTCP) based off the protocols outlined in RFC 3550.
 * 
 * Ideally, this library will implement the entirety of the standard.
 * However, the functionality of this library will be implemented on
 * an as-needed basis. As a result, things may not work/be as robust
 * as the specification. In addition, things may change quickly as issues 
 * are discovered.
 *  
**/
#include <stdint.h>
#include <sys/types.h>
/**
 * Important constants within RTP & RTCP
**/
#define RTP_VERSION 2
#define MAX_DATAGRAM_SIZE 508 // the number of octects that won't result in fragmentation of the datagram 576 (min MTU)  - 60 (max IP header) - 8 (UDP header) = 508
// packet types
#define RTCP_PACKET_SENDER_REPORT 	200
#define RTCP_PACKET_RECEIVER_REPORT 	201
#define RTCP_PACKET_SOURCE_DESCRIPTION 	202
#define RTCP_PACKET_BYE			203
#define RTCP_PACKET_APP			204
// SDES item types
#define SDES_ITEM_END   0	// end of SDES item list
#define SDES_ITEM_CNAME 1	// canonical name
#define SDES_ITEM_NAME	2	// user name
#define SDES_ITEM_EMAIL 3	// email
#define SDES_ITEM_PHONE 4	// phone number
#define SDES_ITEM_LOC 	5	// location
#define SDES_ITEM_TOOL	6	// name of application
#define SDES_ITEM_NOTE	7	// notice about source
#define SDES_ITEM_PRIV	8	// private extensions

/** 
 * Bitmask operations for getting/setting individual fields within
 * RTP header bitfield.
 * 
 * These operations should be applied to a 16-bit unsigned integer
 * (uint16_t) that represents the first 16 bits of a RTP Header 
**/
	#define GET_VERSION(bitfield) (bitfield >> 14) 
	#define GET_PADDING(bitfield) ((bitfield >> 13) & 0x1) // 001
	#define GET_EXTENSION(bitfield) ((bitfield >> 12) & 0x1)
	#define GET_CSRC_COUNT(bitfield) ((bitfield >> 8) & 0xF) // 1111
	#define GET_MARKER(bitfield) ((bitfield >> 7) & 0x1)
	#define GET_PAYLOAD_TYPE(bitfield) (bitfield & 0x7F) // 1111111
	#define VERSION_MASK(version) (version << 14)
	#define PADDING_MASK(padding) ((padding & 0x1) << 13)
	#define EXTENSION_MASK(extension) ((extension & 0x1) << 12)
	#define CSRC_COUNT_MASK(count) ((count & 0xF) << 8)
	#define MARKER_MASK(marker) ((marker & 0x1) << 7)
	#define PAYLOAD_TYPE_MASK(payload) (payload & 0x7F)
/**
 * A structure representing a participant & info about them.
**/
struct participant_info {
	uint32_t ssrc; 		  // the ssrc identifier
	char* host;    		  // the host identifier (ip address as string)
	int rtp_port;  		  // the port on which the participant accepts RTP packets
	int rtcp_port;		  // the port on which the participant accepts RTCP packets
	int num_received_packets; // number of packets received from participant
	int num_packets_expected; // number of packets expected to be received (highest_seq - init_seq)
	int num_packets_lost;     // number of packets that weren't received for any reason (expected - received)
	int highest_seq;	  // the highest sequence number received so far
	int init_seq;		  // the initial sequence number (should be random)
	int cycles;		  // the number of times seq wraps around (since it is only 16 bits)
	// todo: more statistics will be added as needed
};
/**
 * A structure representing an RTP session.
**/
struct rtp_session {
	uint32_t ssrc;				// your synchronization source identifier
	int rtp_port;				// the port that will accept incoming RTP packets
	int rtcp_port;  			// the port that will accept incoming RTCP packets
	int rtp_sock;				// the socket established to listen for rtp on a random port
	int rtcp_sock; 				// the socket established to listen for rtcp on a random port
	int num_participants; 			// the number of participants in the session
	struct participant_info* participants;	// the individuals participating in the session	
	uint16_t init_seq; 				// the initial (random) sequence number for outgoing rtp packets
	uint16_t last_seq; 				// the last sequence number sent out
	uint32_t init_timestamp;			// the initial (random) value used for the relative timestamp of outgoing payload
	time_t t_last_rtcp_packet_sent; 		// the time the last rtcp packet was sent
	int initial_rtcp;				// 1 if an rtcp packet has not yet been sent
};

/**
 * A structure that comprises an RPT profile.
**/
struct rtp_profile {
	unsigned int uses_extension;	 // 0 if the profile does not make use of the header extension else 1
	unsigned int payload_type;	 // the type of the payload
	unsigned int extension_length;   // the size of the extension
	unsigned int max_payload_length; // the maximum size a payload should be or 0 if there is no max size
};
/**
 * A bit field used to represent the RTP packet header.
**/
struct rtp_header {
	uint16_t bitfields;		// the first 16 bits of the rtp header
	uint16_t sequence_number; 	// the number of the rtp packet sent in the sequence. increment by 1 for each packet
	uint32_t timestamp;	  	// the timestamp of the packet
	uint32_t ssrc;		  	// the synchronization source  
	uint32_t csrc[1];		// optional list of contributing sources 
};
/**
 * RTP packet
**/
struct rtp_packet {
	struct rtp_header header;	// the RTP header
	uint8_t payload[1];		// the payload
};
/**
 * Represents an RTP header extension
**/
struct rtp_header_extension {
	uint16_t profile_specific; 	// 16 bits to be defined by the profile for anything
	uint16_t length;		// 16 bits - number of words (4 bytes) in extension (excluding this header)
	uint32_t data[1]; 		// the extension data
};
/**
 * A bit field used to represent the fixed-length RTCP header
**/
struct rtcp_header_fixed {
	unsigned int version: 2;	// RTP version
	unsigned int padding: 1;	// indicates whether the packet contains padding
	unsigned int rr_count: 5; 	// number of reception reports in this packet
	unsigned int packet_type: 8;    // the type of rtcp packet
	uint16_t length;		// the length of the packet in 32-bit words - 1
};
/**
 * Contains information about a sender for the SR
**/
struct rtcp_sender_info {
        uint64_t ntp_timestamp;         // the wallclock time when this report was sent
        uint64_t rtp_timestamp;         // time above, but in same units & random offset as the RTP packet timestamps
        uint32_t sender_packet_count;   // the number of rtp packets sent thus far
        uint32_t sender_octet_count;   // the number of octets sent in all payloads thus far
};
/**
 * Holds info about data received from a given SSRC
**/
struct rtcp_reception_report {
        uint32_t ssrc;                          // the ssrc of the source to which this report pertains
        uint8_t  fraction_lost;                 // the fraction of rtp packets from this source lost since previous SR or RR was sent
        unsigned int cum_num_packets_lost: 24;  // the number of rtp packets lost since beginning
        uint32_t ext_highest_seq_num;           // the highest sequence number received (extended in some way??)
        uint32_t interarrival_jitter;           // some formula for jitter
        uint32_t last_sr_timestamp;             // middle 32 bits of most recent SR ntp_timestamp
        uint32_t last_sr_delay;                 // the delay between receiving last SR packet from source SSRC & sending this (1/65536 sec)

};
/**
 * RTCP sender report (SR)
**/
struct rtcp_sender_report {
	uint32_t ssrc;						// the ssrc of the sender
	struct rtcp_sender_info sender_info;			// sender info
	struct rtcp_reception_report reception_report[1]; 	// optional list of reception reports
	// todo implement a SR & RR profile extension
};
/**
 * RTCP receiver report (RR)
**/
struct rtcp_receiver_report {
	uint32_t ssrc; 						// receiver generating this report
	struct rtcp_reception_report reception_report[1]; 	// list of reception reports.
	// todo implement a SR & RR profile extension
};
/**
 * RTCP SDES item
**/
struct sdes_item {
	uint8_t type;		// the type of the item
	uint8_t length;		// the length of the text field (in octets)
	char text[1];		// the text.
};
/**
 * RTCP SDES chunk.
**/
struct rtcp_sdes_chunk {
	uint32_t src; 			// either a CSRC or SSRC
	struct sdes_item item[1];	// note: this list must be terminated by a null octet	 		
};
/**
 * RTCP Source Description (SDES) 
**/
struct rtcp_source_description {
	struct rtcp_sdes_chunk	chunks[1];	// sdes chunks
};
/**
 * RTCP BYE 
**/
struct rtcp_bye {
	uint32_t src;				// the (C/S)SRC
	uint8_t length;				// the length of the reason for leaving (in octets)
	char reason[1];				// the reason for leaving
};
/**
 * RTCP packet
**/
struct rtcp_packet {
	struct rtcp_header_fixed header;
	union {
		struct rtcp_bye bye;
		struct rtcp_source_description sdes;
		struct rtcp_receiver_report rr;
		struct rtcp_sender_report sr;
	} contents;
};

// start function declarations

/**
 * Initializes an rtp session. On success rtp_session will be populated with necessary values.
 * param: (struct rtp_session*) -> A pointer to an empty rtp_session.
 * return: (int)		-> 1 on success, 0 on failure 
**/
int rtp_session_init(struct rtp_session* session);

/**
 * Sends an RTP packet to all of the participants.
 * param: session (struct rtp_session*)      -> an active rtp_session
 * param: packet  (struct rtp_packet*)       -> an rtp packet
 * param: packet_length (struct rtp_packet*) -> the length of the packet (header + ext + payload + padding + anything else) in octets
 * return: (int)                             -> 1 on success, 0 on failure
**/
int rtp_send(struct rtp_session* session, struct rtp_packet* packet, size_t packet_length);

/**
 * Calculates the size of an RTP header. 
 * param: (rtp_header*) header -> the rtp header
 * return: (size_t) the size of the RTP header.
 *
 * (impl. note): we cannot simply assume a header is 16 octets b/c 
 * variable length list of csrc's and possible header extension.
 * Smallest possible rtp_header is 12 octets.
**/
size_t rtp_header_size(struct rtp_header* header);

/**
 * Calculates the size of the payload within an RTP packet
 * param: (rtp_packet*) packet -> the rtp packet
 * param: (size_t) packet_size -> the size of the total packet
 * return: (size_t) the size of the payload.
**/
size_t rtp_payload_size(struct rtp_packet* packet, size_t packet_size);

/**
 * Finds a participant in the RTP session participant list via ssrc
 * param: (rtp_session*) session -> an active RTP session
 * param: (uint32_t) ssrc        -> the ssrc of the participant
 * return: (int)                 -> the index of the participant if found, otherwise -1
**/
int find_participant(struct rtp_session* session, uint32_t ssrc);

/**
 * Adds a participant to the RTP session participant list
 * param: (rtp_session*) session -> an active RTP session
 * param: (uint32_t) ssrc        -> the ssrc of the participant
**/
void add_participant(struct rtp_session* session, uint32_t ssrc);

/**
 * Removes a participant from the RTP session participant list
 * param: (rtp_session*) session -> an active RTP session
 * param: (uint32_t) ssrc        -> the ssrc of the participant
 * return: (int)                 -> 1 if remove was successful, 0 if ssrc wasn't found
**/
int remove_participant(struct rtp_session* session, uint32_t ssrc);

/**
 * Prints a list of all the current participants in an RTP session
 * param: (rtp_session*) -> an active RTP session
**/
void print_participants(struct rtp_session* session);
#endif

