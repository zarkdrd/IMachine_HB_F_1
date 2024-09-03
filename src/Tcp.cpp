/*************************************************************************
    > File Name: src/Tcp.cpp
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-21
 ************************************************************************/

#include "Tcp.h"
#include "Log_Message.h"

TcpClient::TcpClient(string IP, unsigned short port)
{
	sockfd = -1;
	ServerIP = IP;
	_port = port;
	isOpen = false;
	ConnectLen  = 0;
	if(port != 0){
		pthread_create(&sockID, NULL, ConnectThread, this);
	}
}
TcpClient::~TcpClient()
{
	if(isOpen == true){
		Close();
	}
}
bool TcpClient::Open()
{
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0){
		log_message(ERROR, "socket: %s", strerror(errno));
		return false;
	}
	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(_port);
	server.sin_addr.s_addr = inet_addr(ServerIP.c_str());

	if(connect(sockfd, (struct sockaddr*)&server, sizeof(server)) < 0){
		if((ConnectLen <= 0) || (ConnectLen > 600)){
			log_message(ERROR, "Connect [%s : %d] err: %s", ServerIP.c_str(), _port, strerror(errno));
			ConnectLen = 0;
		}
		ConnectLen++;
		close(sockfd);
		return false;
	}
	isOpen = true;
	ConnectLen = 0;
	log_message(INFO, "Connect [%s : %d] successful", ServerIP.c_str(), _port);
	return true;
}

bool TcpClient::Close()
{
	pthread_cancel(sockID);
	if(isOpen == true){
		close(sockfd);
	}
	return true;
}

int TcpClient::Send(void* buffer, int len)
{
	if(!isOpen){
		return false;
	}
	if(send(sockfd, buffer, len, MSG_NOSIGNAL) < 0){
		log_message(ERROR, "The TcpServer disconnect: %s", strerror(errno));
		close(sockfd);
		isOpen = false;
		return -1;
	}
	return true;
}

int  TcpClient::Recv(void* buffer, int len)
{
	if(!isOpen){
		return -1;
	}
	int RecvLen = recv(sockfd, buffer, len, 0);
	if(RecvLen <= 0){
		log_message(ERROR, "The TcpServer disconnect: %s", strerror(errno));
		close(sockfd);
		isOpen = false;
		return -1;
	}

	return RecvLen;
}

int  TcpClient::Recv(void* buffer, int len, int sec, int msec)
{
	if(!isOpen){
		return -1;
	}
	int recvLen;
	fd_set rfds;
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = msec * 1000;

	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);
	int retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);
	if (retval < 0) {
		log_message(ERROR, "select : %s", strerror(errno));
		return -1;
	}else if (retval == 0) {
		return 0;
	}
	recvLen = recv(sockfd, buffer, len, 0);
	if(recvLen <= 0){
		log_message(ERROR, "The TcpServer disconnect: %s", strerror(errno));
		close(sockfd);
		isOpen = false;
		return -1;
	}
	return recvLen;
}

int  TcpClient::GetHandleCode()
{
	return sockfd;
}

bool TcpClient::GetStatus()
{
	return isOpen;
}

void *ConnectThread(void *arg)
{
	class TcpClient *p = (class TcpClient *)arg;
	sleep(10);
	while(1){
		if(p->isOpen == false){
			p->Open();
		}
		sleep(1);
	}

	return NULL;
}

