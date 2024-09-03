/*************************************************************************
    > File Name: include/Udp.h
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-25
 ************************************************************************/

#ifndef _INCLUDE_UDP_H_
#define _INCLUDE_UDP_H_

#include <iostream>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "Communication.h"

class UdpClient : public Communication{
    public:
        UdpClient(string IP, int Port);
        ~UdpClient();

		virtual bool Open(void) override;
		virtual bool Close(void) override;
		virtual int  Send(void* buffer, int len) override;
		virtual int  Recv(void* buffer, int len) override;
		virtual int  Recv(void* buffer, int len, int sec, int msec) override;
		virtual int  GetHandleCode() override;
		virtual bool GetStatus() override;

    public:
        volatile bool isOpen;
        volatile int sockfd;
        int _Port;
        string ServerIP;
        socklen_t addrlen;
        struct sockaddr_in serveraddr;
};

#endif //_INCLUDE_UDP_H_
