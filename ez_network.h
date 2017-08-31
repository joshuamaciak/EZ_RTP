#ifndef EZ_NETWORK_H
#define EZ_NETWORK_H
#include <sys/types.h>
/**
 * EZ_NETWORK  
 * Author: Joshua Maciak
 * Date: August 13 2017
 * 
 * EZ_NETWORK is a header full of various convenience functions
 * for networking. This header will contain a bunch of functions
 * that will take a lot of the leg-work out of communicating via 
 * sockets, which will mean less messy error-prone code in the EZ_RTP
 * library. It also provides a sense of module-ization. If later I need
 * to add something like packet-level encryption (DTLS) via OpenSSL,
 * I can simply switch out the implementations (in theory)
**/

/**
 * Attempts to bind an address to a socket.
 * param: sock (int)         -> a socket file descriptor
 * param: family (int)       -> an address family
 * param: address (int)      -> an address
 * param: port (int*)  [out] -> the port. if 0, port will be ran
domly selected & this parameter will be set to its value on retu
rn.
   return: (int)             -> 1 on success, 0 on failure.
**/
int ez_bind(int sock, int family, long address, int* port);
/**
 * Sends a packet to a host on the specified port.
 * param: sock (int)      -> a socket file descriptor
 * param: data (void*)    -> a buffer containing the data to send
 * param: length (size_t) -> the length of the data buffer
 * param: addr_family (int) -> the address family AF_INET | AF_INET6
 * param: host (char*)    -> the ipv4 address to send to in dot notation
 * param: port (int)      -> the port to send to
 * return: (int)          -> 1 on success, 0 on failure
**/
int ez_sendto(int sock, void* data, size_t length, int addr_family, char* host, int port);

/**
 * Attempts to read a datagram from a UDP socket. Note: this function does NOT block.
 * param: sock (int)      -> a socket file descriptor
 * param: buf (void*)     -> a region of memory where the datagram will be stored
 * param: length (size_t) -> the length of the supplied buffer
 * return: number of bytes read on success, -1 on failure
**/
int ez_recv_noblock(int sock, void* buf, size_t length);
#endif
