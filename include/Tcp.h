/*************************************************************************
    > File Name: include/Tcp.h
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-25
 ************************************************************************/

#ifndef _INCLUDE_TCP_H_
#define _INCLUDE_TCP_H_

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

class TcpClient : public Communication{
    public:
        TcpClient(string IP, unsigned short port);
        ~TcpClient();

		virtual bool Open(void) override;
		virtual bool Close(void) override;
		virtual int  Send(void* buffer, int len) override;
		virtual int  Recv(void* buffer, int len) override;
		virtual int  Recv(void* buffer, int len, int sec, int msec) override;
		virtual int  GetHandleCode() override;
		virtual bool GetStatus() override;

    public:
        volatile bool isOpen;
    private:
        int sockfd;
        string ServerIP;
        unsigned short _port;
        struct sockaddr_in server;

        int ConnectLen;
        pthread_t sockID;
};

void *ConnectThread(void *arg);

#endif //_INCLUDE_TCP_H_
