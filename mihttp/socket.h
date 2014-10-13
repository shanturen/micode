#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include "buffer.h"
#include <string.h>
using std::string;

class address : public sockaddr
{
public:
	address()
	{
		sockaddr *sa= this; // work without cast?
		struct sockaddr_in *sai = reinterpret_cast<sockaddr_in *>(sa);
		memset(sai, 0, sizeof(sockaddr_in));
	}
	address(const string &ip, int port) 
	{
		sockaddr *sa= this; // work without cast?
		struct sockaddr_in *sai = reinterpret_cast<sockaddr_in *>(sa);
		memset(sai, 0, sizeof(sockaddr_in));
		sai->sin_family = AF_INET;
		sai->sin_port = htons(port);
		sai->sin_addr.s_addr = inet_addr(ip.c_str());
	}
	//sockaddr *operator &() { return static_cast<sockaddr *>(this); };
	string ip() const
	{
		const sockaddr_in *sai = reinterpret_cast<const sockaddr_in *>(static_cast<const sockaddr *>(this));
		return inet_ntoa(sai->sin_addr);
	}
	int port() const
	{
		const sockaddr_in *sai = reinterpret_cast<const sockaddr_in *>(static_cast<const sockaddr *>(this));
		return ntohs(sai->sin_port);
	}
	int length() const {return sizeof(sockaddr); }
};

int tcp_read_ms(int sock, void *data, int length, int msecs);
int tcp_write_ms(int sock, const void *data, int length, int msecs);
int tcp_read_ms(int sock, buffer &buf, int length, int msecs);
int tcp_read_ms_once(int sock, buffer &buf, int length, int msecs);
int check_alive(int sock);
