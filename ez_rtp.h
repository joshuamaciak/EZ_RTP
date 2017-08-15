#ifndef RTP_H
#define RTP_H
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

#define RTP_VERSION 2

/**
 * A bit field used to represent the RTP packet header.
**/
struct rtp_header {
	unsigned int version: 2;        // RTP version
	unsigned int padding: 1;        // indicates whether the header contains padding
	unsigned int extension: 1;      // indicates whether the header contains an extension
	unsigned int csrc_count: 4;     // the number of csrc id's that follow the fixed header
	unsigned int marker: 1;         // determined by the RTP profile
	unsigned int payload_type: 7;   // identifies the format of the payload 
	uint16_t sequence_number; 	// the number of the rtp packet sent in the sequence. increment by 1 for each packet
	uint32_t timestamp;	  	// the timestamp of the packet
	uint32_t ssrc;		  	// the synchronization source  
	uint32_t cscr[1];		// optional list of contributing sources 
};

/**
 * Represents an RTP header extension
**/
struct rtp_header_extension {
	uint16_t profile_specific; 	// 16 bits to be defined by the profile for anything
	uint16_t length;		// 16 bits - number of words (4 bytes) in extension (excluding this header)
	uint32_t* data; 		// the extension data
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
	uint32_t ssrc: 32; 		// the synchronization source 
};
/**
 * RTCP sender report (SR) packet
**/
struct rtcp_sender_report {

};
#endif
