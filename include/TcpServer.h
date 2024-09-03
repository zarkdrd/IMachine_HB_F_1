

/*************************************************************************
    > File Name: include/TcpServer.h
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-10-30
 ************************************************************************/

#ifndef _INCLUDE_TCPSERVER_H_
#define _INCLUDE_TCPSERVER_H_

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

using namespace std;

class TcpServer{
	public:
		TcpServer(unsigned short port);
		~TcpServer();
		bool Open();
		bool Close();
		bool WriteByte(void* buffer, int len);
		int  ReadByte(void* buffer, int len, int sec, int msec);
		int  RecvByte(void* buffer, int len);
		bool Accept();
	public:
		bool isOpen;
        int newfd;
	private:
		int sockfd;
		pthread_t sockID;
		unsigned short _port;
};

void *AcceptThread(void *arg);

#endif //_INCLUDE_TCPSERVER_H_

// /*************************************************************************
// 	> File Name: include/TcpServer.h
// 	> Author: ARTC
// 	> Descripttion:
// 	> Created Time: 2023-10-30
//  ************************************************************************/

// #ifndef _INCLUDE_TCPSERVER_H_
// #define _INCLUDE_TCPSERVER_H_

// #include <iostream>
// #include <fcntl.h>
// #include <pthread.h>
// #include <string.h>
// #include <stdlib.h>
// #include <sys/time.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netinet/tcp.h>
// #include <arpa/inet.h>
// #include <list>
// #include <thread>

// using namespace std;

// class TcpServer
// {
// public:
// 	TcpServer(unsigned short port);
// 	~TcpServer();
// 	bool Open();
// 	bool Close();
// 	int WriteByte(void *buffer, int len);
// 	int ReadByte(void *buffer, int len, int sec, int msec);
// 	int RecvByte(void *buffer, int len);
// 	bool Accept();
// 	bool Savefd(int clientfd);

// public:
// 	bool isOpen;
// 	// 定义一个结构体来保存客户端套接字和相关数据
// 	// struct ClientInfo
// 	// {
// 	// 	int socketfd;	   // 套接字文件描述符
// 	// 	string clientAddr; // 客户端地址信息
// 	// 					   // 可以根据需要添加其他信息，比如客户端的ID、状态等
// 	// };
// 	// int max_sd;
// 	// fd_set readfds;
// 	// ClientInfo newclient;
// 	// list<ClientInfo> clients;
// 	// list<ClientInfo>::iterator it;
// 	// bool Deletefd(list<ClientInfo>::iterator it);

// private:
// 	int sockfd, newfd;
// 	string ipaddr;
// 	pthread_t sockID;
// 	unsigned short _port;
// 	std::thread SelectThread;
// };

// void *AcceptThread(void *arg);

// #endif //_INCLUDE_TCPSERVER_H_
