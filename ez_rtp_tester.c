#include <stdio.h>
#include <stdlib.h>
#include "ez_rtp.h"
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#define RTP_PAYLOAD_MPEG4 34

int is_rtp_header_valid();
void print_rtp_packet(struct rtp_packet* rtp_packet, size_t packet_size_bytes);
void print_rtp_session(struct rtp_session* session);
int rtp_header_to_n(struct rtp_header* src, struct rtp_header* dest);
void read_nal_bitstream_send(struct rtp_session* session, char* filename, int port);
void fpeek(uint8_t* buf, int bytes, FILE* file);
uint32_t sum_next_32_bits(FILE* file);
uint32_t sum_next_24_bits(FILE* file);
int send_rtp_packet_test(struct rtp_session* session, int port, uint8_t* nal_buf, size_t cur_nal_buf_size, int seq_num);
void print_buf(uint8_t* buf, size_t len);
void log_payload_sent(int packet_num, uint8_t* buf, int length) {
	FILE* log = fopen("sent.log", "a");
	fwrite(&packet_num, sizeof(int), 1, log);
	fwrite(buf, sizeof(uint8_t), length, log);
	fclose(log);
}
void log_payload_recv(int packet_num, uint8_t* buf, int length) {
        FILE* log = fopen("recv.log", "a");
        fwrite(&packet_num, sizeof(int), 1, log);
        fwrite(buf, sizeof(uint8_t), length, log);
	fclose(log);
}
void print_buf(uint8_t* buf, size_t len) {
	for(int i = 0; i < len; ++i) {
		printf("%02X ",(unsigned)buf[i]);
	}
}
int main(int argc, char** argv) {
	struct rtp_session my_session;
	if(rtp_session_init(&my_session) == 0) {
		printf("Error: failed to initialize RTP session.\n");
		return -1;
	}
	printf("Successfully initiated RTP session.\n");
	print_rtp_session(&my_session);
	
	if(argc == 3 && strcmp(argv[1], "r") == 0) {
		const uint8_t NAL_START_CODE[4] = {0x0, 0x0, 0x0, 0x1};
		printf("Waiting for rtp packets...\n");
		
		FILE* out_file = fopen(argv[2], "wb");
	
		int total_nal_bytes_recvd = 0;
		int total_bytes_recvd = 0;
		int last_seq_read = 0;
		int expected = 0;
		while(1) {
			const size_t max_buf_len = 64000;
			uint8_t buf[max_buf_len];
			int length = 0;
			if((length = ez_recv_noblock(my_session.rtp_sock, buf, max_buf_len)) != -1) {
				printf("Received RTP packet. size:%zu\n", length);
				struct rtp_packet* recvd_packet = buf;
				print_buf(recvd_packet, length);
				last_seq_read = ntohs(recvd_packet->header.sequence_number);
				log_payload_recv(last_seq_read, recvd_packet->payload, length - 16);
				if(expected != last_seq_read) {
					printf("ERROR::: expected %d, read %d\n", expected, last_seq_read);
				}
				printf("Writing payload of rtp_packet seq_num=%" PRIu16 "\n", ntohs(recvd_packet->header.sequence_number));
				fwrite(NAL_START_CODE, sizeof(uint8_t), 4, out_file);

				
				fwrite(recvd_packet->payload, sizeof(uint8_t), length - 16, out_file);
				total_nal_bytes_recvd += length - 16;	
				total_bytes_recvd += length;
				// THIS IS HARDCODED & DANGEROUS REMOVE IT!!!
				++expected;
			}
			free(buf);
//			print_rtp_session(&my_session);
	//		printf("Total nal bytes recvd: %d\n", total_nal_bytes_recvd);
			if(last_seq_read == 8734) break;	
	}
		printf("Total nal bytes recvd: %d\n", total_nal_bytes_recvd);
		printf("bytes recvd: %d\n", total_bytes_recvd);
		fclose(out_file);
		return 0;
	} else if(argc == 3 && strcmp(argv[1], "s") == 0) {
		
		read_nal_bitstream_send(&my_session, "out.264", atoi(argv[2]));
	}	
	return 0;
}
/**
 * Converts fields of rtp_header to network byte ordering
**/
int rtp_header_to_n(struct rtp_header* dest, struct rtp_header* const src) {
	*dest = *src;
	dest->bitfields = htons(src->bitfields);
	dest->sequence_number = htons(src->sequence_number);
	dest->timestamp	      = htonl(src->timestamp);
	dest->ssrc	      = htonl(src->ssrc);
	for(int i = 0; i < GET_CSRC_COUNT(src->bitfields); ++i) {
		dest->csrc[i] = htonl(src->csrc[i]);
	}
	return 1;
}
int send_rtp_packet_test(struct rtp_session* session, int port, uint8_t* nal_buf, size_t cur_nal_buf_size, int seq) {
	struct rtp_header header;
	header.bitfields = VERSION_MASK(2) | PADDING_MASK(0) | EXTENSION_MASK(0) | CSRC_COUNT_MASK(1) | MARKER_MASK(0) | PAYLOAD_TYPE_MASK(RTP_PAYLOAD_MPEG4);
	header.csrc[0] = 0x001111;
	header.ssrc = 0x001010;
	header.sequence_number = seq;
	// todo: need to adjust based on csrc list size
	size_t packet_size = sizeof(struct rtp_header) + cur_nal_buf_size;
	printf("NAL unit length: %zu packet header: %zu total packet size: %zu\n", cur_nal_buf_size, sizeof(struct rtp_header), packet_size);                   // send data
	struct rtp_packet* new_packet = malloc(packet_size);
	memset(new_packet, 0, sizeof(uint8_t) * packet_size);
	
	memcpy(new_packet->payload, nal_buf, cur_nal_buf_size);
	rtp_header_to_n(&(new_packet->header), &header);
	
	print_buf(new_packet, packet_size);
	if(ez_sendto(session->rtp_sock, (void*) new_packet, packet_size, AF_INET, "127.0.0.1", port) == 1) {
		log_payload_sent(seq, nal_buf, cur_nal_buf_size);
		printf("Sent packet! seq_num:%" PRIu16 "\n", seq);
	} else {
		printf("Failed to send on port %d. Errno: %d", port, errno);
	}	
	free(new_packet);
	return 1;
}
/**

 * reads a nal bitstream & sends it via rtp
**/
void read_nal_bitstream_send(struct rtp_session* session, char* filename, int port) {
	FILE* bitstream = fopen(filename, "rb");
	struct stat buf;
	fstat(fileno(bitstream), &buf);
	int file_size = buf.st_size;
	if(bitstream == NULL) {
		printf("Invalid file.\n");
		return;
	}
	const size_t max_nal_buf_size = 50000;
	uint8_t nal_buf[max_nal_buf_size];
	size_t cur_nal_buf_size = 0;	
	uint8_t cur_byte;
	memset(nal_buf, 0, sizeof(uint8_t) * max_nal_buf_size);
	int num_bytes_read = 0;
	int total_nal_bytes_sent = 0;
	int num_packets_sent = 0;
	while(num_bytes_read < file_size) {
		// if next 3 or 4 bytes is an end code
		// send current nal buf (1 full nal unit
		// else keep reading bytes into buf
		
		int do_send = 0;
		if(sum_next_24_bits(bitstream) == 0x000001) {
			num_bytes_read += 3;
			fseek(bitstream, 3, SEEK_CUR);
			do_send = 1;
		} else if(sum_next_32_bits(bitstream) == 0x00000001) {
			num_bytes_read += 4;
                        fseek(bitstream, 4, SEEK_CUR);
			do_send = 1;
		}

		if(do_send && cur_nal_buf_size > 0) {
			send_rtp_packet_test(session, port, nal_buf, cur_nal_buf_size, num_packets_sent);
			total_nal_bytes_sent += cur_nal_buf_size;
			num_packets_sent++;
			cur_nal_buf_size = 0;
		} 
		if( num_bytes_read < file_size) {
			fread(&cur_byte, sizeof(uint8_t), 1, bitstream);
			nal_buf[cur_nal_buf_size] = cur_byte;
			cur_nal_buf_size++;
			num_bytes_read++;
		}
			
	}
	// send final nal packet at EOF
        send_rtp_packet_test(session, port, nal_buf, cur_nal_buf_size, num_packets_sent);
	total_nal_bytes_sent += cur_nal_buf_size;
	printf("Total size of all nal units sent: %d\n", total_nal_bytes_sent);  
}
uint32_t sum_next_24_bits(FILE* file) {
	uint8_t buf[3];
	fpeek(buf, 3, file);
	return buf[0] + buf[1] + buf[2];
}
uint32_t sum_next_32_bits(FILE* file) {
        uint8_t buf[4];
        fpeek(buf, 4, file);
        return buf[0] + buf[1] + buf[2] + buf[3];
}
void fpeek(uint8_t* buf, int bytes, FILE* file) {
	fread(buf, sizeof(uint8_t), bytes, file);
	fseek(file, -bytes, SEEK_CUR);	
}
void print_rtp_session(struct rtp_session* session) {
	printf("- RTP Session - \n");
	printf("SSRC:%" PRIu32 "\nRTP port:%d\nRTCP port:%d\nParticipants:%d\n", session->ssrc, session->rtp_port, session->rtcp_port, session->num_participants);	
}
void print_rtp_packet(struct rtp_packet* packet, size_t packet_size_bytes) {
	struct rtp_header header = packet->header;
	printf("- RTP packet - \n");
	printf("Size: %ld octets\n", packet_size_bytes);
	printf("* Header *\n");
	printf("Size: %ld octets\n", sizeof(struct rtp_header) + (GET_CSRC_COUNT(header.bitfields) - 1) * 4);
	printf("version:%d\npadding:%d\nextension:%d\ncsrc_count:%d\nmarker:%d\npayload_type:%d\nsequence_number:%d\ntimestamp:%d\nssrc:%d\n", GET_VERSION(header.bitfields), GET_PADDING(header.bitfields), GET_EXTENSION(header.bitfields), GET_CSRC_COUNT(header.bitfields), GET_MARKER(header.bitfields), GET_PAYLOAD_TYPE(header.bitfields), header.sequence_number, header.timestamp, header.ssrc);
	for(int i = 0; i < GET_CSRC_COUNT(header.bitfields); ++i) printf("csrc[%d]: %d\n", i, header.csrc[i]);
}

/**
 * Checks to see if an RTCP packet is valid (note: a packet can consist 
 * of multiple packets, so all must be sanitized)
**/

int is_rtcp_packet_valid(struct rtcp_packet* packet, int packet_size_words) {
	// version must be 2
	if(packet->header.version != RTP_VERSION) return 0;
	// first packet must be SR or RR
	if((packet->header.packet_type != RTCP_PACKET_SENDER_REPORT) || (packet->header.packet_type != RTCP_PACKET_RECEIVER_REPORT)) {
		return 0;
	}	
	// first packet shouldn't have padding
	if(packet->header.padding == 1) return 0;
	// ensure individual packet length adds up to total packet size
	struct rtcp_packet* cur = packet;
	struct rtcp_packet* end = (struct rtcp_packet*) ((uint32_t*) packet + packet_size_words);
	do cur = (struct rtcp_packet*) ((uint32_t *)cur + cur->header.length + 1);
	while(cur < end && cur->header.version == RTP_VERSION);
	if(cur != end) return 0;
	return 1;	
}

/**
 * Checks to see if the packet contains a valid RTP header
**/
int is_rtp_header_valid(struct rtp_packet* packet, int packet_size, int header_size, struct rtp_profile* profile) {
	struct rtp_header header = packet->header;
	if(GET_VERSION(header.bitfields) != RTP_VERSION) return 0;
	if(GET_PAYLOAD_TYPE(header.bitfields) != profile->payload_type) return 0;
	if(GET_EXTENSION(header.bitfields) != profile->uses_extension) return 0;
	if(GET_PADDING(header.bitfields) == 1) {
		uint8_t num_padding_octets = *((uint8_t*)packet + (packet_size - 1));
		if(num_padding_octets >= packet_size - header_size) return 0;
	}
	return 1;	 
}
