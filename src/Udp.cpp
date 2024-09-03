/*************************************************************************
    > File Name: src/UdpClient.cpp
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-22
 ************************************************************************/

#include "Udp.h"
#include "Log_Message.h"


UdpClient::UdpClient(string IP, int Port)
{
	ServerIP = IP;
	_Port = Port;
	isOpen = false;
	addrlen = sizeof(serveraddr);
}

UdpClient::~UdpClient()
{
	Close();
}

bool UdpClient::Open()
{
	if(isOpen == false){
		close(sockfd);
	}

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0){
		log_message(ERROR, "Created socket err: %s", strerror(errno));
		return false;
	}

	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(_Port);
	serveraddr.sin_addr.s_addr = inet_addr(ServerIP.c_str());

	isOpen = true;

	return true;
}

bool UdpClient::Close()
{
	if(sockfd < 0){
		close(sockfd);
	}
	return false;
}
int UdpClient::Send(void* buffer, int len)
{
	return sendto(sockfd, buffer, len, 0, (struct sockaddr*)&serveraddr, addrlen);
}

int  UdpClient::Recv(void* buffer, int len)
{
	return recvfrom(sockfd, buffer, len, 0, NULL, NULL);;
}

int  UdpClient::Recv(void* buffer, int len, int sec, int msec)
{
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

	int recvLen = recvfrom(sockfd, buffer, len, 0, NULL, NULL);
	if(recvLen <= 0){
		log_message(ERROR, "recvfrom: %s", strerror(errno));
		return -1;
	}
	return recvLen;
}

int  UdpClient::GetHandleCode()
{
	return sockfd;
}

bool UdpClient::GetStatus()
{
	return isOpen;
}
