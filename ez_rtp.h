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
	struct rtcp_reception_report reception_report[1]; 	// list of reception reports.
	// todo implement a SR & RR profile extension
};
#endif