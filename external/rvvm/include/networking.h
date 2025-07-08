/*
networking.h - Network sockets (IPv4/IPv6), Event polling
Copyright (C) 2021  LekKit <github.com/LekKit>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

#ifndef NETWORKING_H
#define NETWORKING_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct net_sock net_sock_t;

typedef struct {
    // Address family (IPv4/IPv6)
    uint16_t type;

    // Port (In host byte order)
    uint16_t port;

    // For IPv4: ip[0].ip[1].ip[2].ip[3]
    uint8_t  ip[16];
} net_addr_t;

#define NET_TYPE_IPV4 0x0
#define NET_TYPE_IPV6 0x1
#define NET_PORT_ANY  0

extern const net_addr_t net_ipv4_any_addr;
extern const net_addr_t net_ipv6_any_addr;
extern const net_addr_t net_ipv4_local_addr;
extern const net_addr_t net_ipv6_local_addr;

// Passed to listen/bind as a shorthand (Picks any free port)
#define NET_IPV4_ANY   (&net_ipv4_any_addr)
#define NET_IPV4_LOCAL (&net_ipv4_local_addr)
#define NET_IPV6_ANY   (&net_ipv6_any_addr)
#define NET_IPV6_LOCAL (&net_ipv6_local_addr)

#define NET_ERR_NONE       0
#define NET_ERR_UNKNOWN    (-1)
#define NET_ERR_BLOCK      (-2)
#define NET_ERR_DISCONNECT (-3)
#define NET_ERR_RESET      (-4)

// Parses IPv6 address string, returns parsed length or 0 on failure
size_t      net_parse_ipv6(net_addr_t* addr, const char* str);

// Parses IPv4 address string, returns parsed length or 0 on failure
size_t      net_parse_ipv4(net_addr_t* addr, const char* str);

// Parses string with IPv4/IPv6 and/or port, returns parsed length or 0 on failure
size_t      net_parse_addr(net_addr_t* addr, const char* str);

// TCP Sockets

net_sock_t* net_tcp_listen(const net_addr_t* addr);
net_sock_t* net_tcp_accept(net_sock_t* listener);
net_sock_t* net_tcp_connect(const net_addr_t* dst, const net_addr_t* src, bool block);
bool        net_tcp_sockpair(net_sock_t* pair[2]);
bool        net_tcp_status(net_sock_t* sock);   // Connected & not yet closed on both sides
bool        net_tcp_shutdown(net_sock_t* sock); // Send EOF (FIN), only recv() works afterwards

int32_t     net_tcp_send(net_sock_t* sock, const void* buffer, size_t size);
int32_t     net_tcp_recv(net_sock_t* sock, void* buffer, size_t size);

// UDP Sockets

net_sock_t* net_udp_bind(const net_addr_t* addr);

size_t      net_udp_send(net_sock_t* sock, const void* buffer, size_t size, const net_addr_t* addr);
int32_t     net_udp_recv(net_sock_t* sock, void* buffer, size_t size, net_addr_t* addr);

// Generic socket operations

net_addr_t* net_sock_addr(net_sock_t* sock);
uint16_t    net_sock_port(net_sock_t* sock);
bool        net_sock_set_blocking(net_sock_t* sock, bool block);
void        net_sock_close(net_sock_t* sock);

// Socket event polling

typedef struct net_poll net_poll_t;

typedef struct {
    uint32_t flags;
    void*    data;
} net_event_t;

// Incoming connection, data received or peer disconnected
// Implicitly watched on all added sockets
#define NET_POLL_RECV 0x1

// Transmission is possible or outbound connect finished
// Check connection success with net_tcp_status() afterwards
#define NET_POLL_SEND 0x2

#define NET_POLL_INF ((uint32_t)-1)

net_poll_t* net_poll_create(void);

bool        net_poll_add(net_poll_t* poll, net_sock_t* sock, const net_event_t* event);
bool        net_poll_mod(net_poll_t* poll, net_sock_t* sock, const net_event_t* event);
bool        net_poll_remove(net_poll_t* poll, net_sock_t* sock);

size_t      net_poll_wait(net_poll_t* poll, net_event_t* events, size_t size, uint32_t wait_ms);

void        net_poll_close(net_poll_t* poll);

#endif
