#ifndef EZ_NETWORK_H
#define EZ_NETWORK_H
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

#endif
