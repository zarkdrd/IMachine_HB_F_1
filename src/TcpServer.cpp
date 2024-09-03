/*************************************************************************
	> File Name: src/TcpServer.cpp
	> Author: ARTC
	> Descripttion:
	> Created Time: 2024-4-16
 ************************************************************************/

#include "TcpServer.h"
#include "Log_Message.h"

TcpServer::TcpServer(unsigned short port)
{
	isOpen = false;
	_port = port;
}

TcpServer::~TcpServer()
{
	Close();
	pthread_cancel(sockID);
	close(sockfd);
}

bool TcpServer::Open()
{
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        log_message(ERROR, "socket : %s", strerror(errno));
		return false;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int keepAlive = 1;     //开启keepalive属性
    int keepIdle = 5;      //如该连接在5秒内没有任何数据往来,则进行探测
    int keepInterval = 1;  //探测时发包的时间间隔为1秒
    int keepCount = 3;     //探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
    setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
    setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

    int opt = 1;
    /**绑定端口号之前，清除之前的绑定**/
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /**禁用nagle算法，防止粘包**/
    int bNodelay = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&bNodelay, sizeof(bNodelay));

    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        close(sockfd);
        log_message(ERROR, "bind : %s", strerror(errno));
		return false;
    }
    if(listen(sockfd, 5) < 0){
        close(sockfd);
        log_message(ERROR, "listen : %s", strerror(errno));
		return false;
    }

	pthread_create(&sockID, NULL, AcceptThread, this);
    log_message(INFO, "Tcp Server [%d] Init successful !", _port);

	return true;
}

bool TcpServer::Close()
{
	isOpen = false;
	shutdown(newfd, SHUT_RD);
	close(newfd);
	return true;
}

bool TcpServer::WriteByte(void* buffer, int len)
{
	if(!isOpen){
		return false;
	}
	send(newfd, buffer, len, MSG_NOSIGNAL);
	return true;
}

int  TcpServer::ReadByte(void* buffer, int len, int sec, int msec)
{
	if(!isOpen){
		log_message(ERROR, "无客户端连接");
		return -1;
	}
	fd_set rfds;
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = msec * 1000;

	FD_ZERO(&rfds);
	FD_SET(newfd, &rfds);
	int retval = select(newfd + 1, &rfds, NULL, NULL, &tv);
	if (retval < 0) {
		log_message(ERROR, "select : %s", strerror(errno));
		return -1;
	}else if (retval == 0) {
		return 0;
	}

	return recv(newfd, buffer, len, 0);
}

int  TcpServer::RecvByte(void* buffer, int len)
{
	return recv(newfd, buffer, len, 0);
}

bool TcpServer::Accept()
{
    struct sockaddr_in client_addr;
    socklen_t sin_size = sizeof(client_addr);
    int new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if(new_fd < 0){
        log_message(ERROR, "accept : %s", strerror(errno));
        return -1;
    }

	if(isOpen){
		Close();
		log_message(INFO, "Close the old connection");
		sleep(1);
	}

    log_message(INFO, "I got a new connection [%d] from (%s:%d)", new_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	newfd = new_fd;
	isOpen = true;

    return new_fd;
}

void *AcceptThread(void *arg)
{
	class TcpServer *Tcp = (class TcpServer *)arg;
	log_message(INFO, "等待客户端进行连接...");
	while(1){
		if(Tcp->Accept() == false){
			sleep(1);
		}
	}
}

// #include "TcpServer.h"
// #include "Log_Message.h"

// TcpServer::TcpServer(unsigned short port)s
// {
// 	isOpen = false;
// 	_port = port;
// }

// TcpServer::~TcpServer()
// {
// 	Close();
// 	pthread_cancel(sockID);
// 	close(sockfd);
// }

// bool TcpServer::Open()
// {
// 	struct sockaddr_in server_addr;

// 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sockfd < 0)
// 	{
// 		log_message(ERROR, "socket : %s", strerror(errno));
// 		return false;
// 	}
// 	memset(&server_addr, 0, sizeof(server_addr));
// 	server_addr.sin_family = AF_INET;
// 	server_addr.sin_port = htons(_port);
// 	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

// 	int keepAlive = 1;	  // 开启keepalive属性
// 	int keepIdle = 5;	  // 如该连接在5秒内没有任何数据往来,则进行探测
// 	int keepInterval = 1; // 探测时发包的时间间隔为1秒
// 	int keepCount = 3;	  // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
// 	setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive));
// 	setsockopt(sockfd, SOL_TCP, TCP_KEEPIDLE, (void *)&keepIdle, sizeof(keepIdle));
// 	setsockopt(sockfd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval));
// 	setsockopt(sockfd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount));

// 	int opt = 1;
// 	/**绑定端口号之前，清除之前的绑定**/
// 	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

// 	/**禁用nagle算法，防止粘包**/
// 	int bNodelay = 1;
// 	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&bNodelay, sizeof(bNodelay));

// 	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
// 	{
// 		close(sockfd);
// 		log_message(ERROR, "bind : %s", strerror(errno));
// 		return false;
// 	}
// 	if (listen(sockfd, 5) < 0)
// 	{
// 		close(sockfd);
// 		log_message(ERROR, "listen : %s", strerror(errno));
// 		return false;
// 	}

// 	FD_ZERO(&readfds);

// 	/*创建线程等待客户端连接*/
// 	pthread_create(&sockID, NULL, AcceptThread, this);

// 	log_message(INFO, "Tcp Server [%d] Init successful !", _port);

// 	return true;
// }

// bool TcpServer::Close()
// {
// 	isOpen = false;
// 	return true;
// }

// int TcpServer::WriteByte(void *buffer, int len)
// {
// 	if (!isOpen)
// 	{
// 		sleep(1);
// 		return false;
// 	}
// 	int flag = 0;
// 	for (it = clients.begin(); it != clients.end(); it++)
// 	{
// 		flag = send(it->socketfd, buffer, len, MSG_NOSIGNAL);
// 	}
// 	// printf("智能节点发送：%d字节\n",flag);
// 	return flag;
// }

// int TcpServer::ReadByte(void *buffer, int len, int sec, int msec)
// {
// 	if (!isOpen)
// 	{
// 		log_message(ERROR, "无客户端连接");
// 		return -1;
// 	}
// 	fd_set rfds;
// 	struct timeval tv;
// 	tv.tv_sec = sec;
// 	tv.tv_usec = msec * 1000;

// 	FD_ZERO(&rfds);
// 	FD_SET(newfd, &rfds);
// 	int retval = select(newfd + 1, &rfds, NULL, NULL, &tv);
// 	if (retval < 0)
// 	{
// 		log_message(ERROR, "select : %s", strerror(errno));
// 		return -1;
// 	}
// 	else if (retval == 0)
// 	{
// 		return 0;
// 	}

// 	return recv(newfd, buffer, len, 0);
// }
// /*
// int TcpServer::RecvByte(void *buffer, int len)
// {

	
// 	// int activity = 0;
// 	// int flag = 0;
// 	// int sd = 0;
// 	// max_sd = 0;
// 	// // 清空readfds集合
// 	// FD_ZERO(&readfds);

// 	// for (it = clients.begin(); it != clients.end(); it++)
// 	// {
// 	// 	sd = it->socketfd;

// 	// 	if (sd > 0)
// 	// 	{
// 	// 		FD_SET(sd, &readfds);
// 	// 	}
// 	// 	if (sd > max_sd)
// 	// 	{
// 	// 		max_sd = sd;
// 	// 	}
// 	// }

// 	// // 设置select超时时间
// 	// struct timeval timeout;
// 	// timeout.tv_sec = 5;
// 	// timeout.tv_usec = 0;

// 	// // 等待直到至少有一个文件描述符准备好进行I/O操作
// 	// activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
// 	// if (activity == 0)
// 	// {
// 	// 	// printf("超时，没有活动:%d\n",activity);
// 	// 	//  超时，没有活动
// 	// 	return 1;
// 	// }
// 	// if (activity == -1)
// 	// {
// 	// }
// 	// for (it = clients.begin(); it != clients.end(); it++)
// 	// {
// 	// 	if (FD_ISSET(it->socketfd, &readfds))
// 	// 	{
// 	// 		flag = recv(it->socketfd, buffer, len, 0);
// 	// 		if (flag == 0)
// 	// 		{
// 	// 			// 客户端关闭连接
// 	// 			log_message(WARN, "客户端[%s]断开连接", it->clientAddr.c_str());
// 	// 			if (!Deletefd(it))
// 	// 			{
// 	// 				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", flag, strerror(errno));
// 	// 				log_message(ERROR, "某个客户端断开连接，等待重连...");
// 	// 				Close();
// 	// 			}
// 	// 			return 1;
// 	// 		}
// 	// 		else
// 	// 		{
// 	// 			log_message(INFO, "接收到[%s]客户端数据", it->clientAddr.c_str());
// 	// 			return flag;
// 	// 		}
// 	// 	}
// 	// }
// }
// */

// bool TcpServer::Accept()
// {
// 	struct sockaddr_in client_addr;
// 	socklen_t sin_size = sizeof(client_addr);
// 	int new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
// 	if (new_fd < 0)
// 	{
// 		log_message(ERROR, "accept : %s", strerror(errno));
// 		return -1;
// 	}

// 	if(isOpen){
// 		Close();
// 		log_message(INFO, "Close the old connection");
// 		sleep(1);
// 	}

// 	log_message(INFO, "I got a new connection [%d] from (%s:%d)", new_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

// 	newfd = new_fd;
// 	// ipaddr = inet_ntoa(client_addr.sin_addr);
// 	// Savefd(new_fd);
// 	isOpen = true;

// 	// if(SelectThread.joinable() == false){
// 	// 	SelectThread = std::thread(Select_Thread, this);
// 	// }
// 	return new_fd;
// }

// bool TcpServer::Savefd(int clientfd)
// {
// 	// newclient.socketfd = clientfd;
// 	// newclient.clientAddr = ipaddr;
// 	// clients.push_back(newclient);
// 	// cout << "保存的客户端数量为：" << clients.size() << endl;

// 	// 将所有客户端套接字添加到readfds集合

// 	// if(clients.empty()){

// 	// }else{
// 	// 	for(it = clients.begin(); it != clients.end(); it++){
// 	// 		if(it->clientAddr == newclient.clientAddr){
// 	// 			// 要删除的客户端地址
// 	// 			std::string addrToRemove = it->clientAddr;
// 	// 			clients.remove_if([&addrToRemove](const ClientInfo& client) {
// 	//     			return client.clientAddr == addrToRemove;
// 	// 			});
// 	// 			break;
// 	// 		}
// 	// 	}
// 	// 	clients.push_back(newclient);
// 	// }

// 	// //遍历链表
// 	// for (it = clients.begin(); it != clients.end(); it++) {
// 	//     cout << it->socketfd << " " << it->clientAddr <<endl;
// 	// }
// 	return true;
// }

// // bool TcpServer::Deletefd(list<ClientInfo>::iterator it)
// // {
// // 	// clients.remove_if([fd](const ClientInfo& s) {
// // 	//     return s.socketfd == fd;
// // 	// });
// // 	shutdown(it->socketfd, SHUT_RD);
// // 	FD_CLR(it->socketfd, &readfds);
// // 	close(it->socketfd);
// // 	clients.erase(it);
// // 	// printf("剩余客户端数量：%d\n",clients.size());
// // 	log_message(INFO, "当前客户端数量[%d]", clients.size());
// // 	if (clients.empty())
// // 	{
// // 		return false;
// // 	}
// // 	else
// // 	{
// // 		it = clients.begin();
// // 	}

// // 	return true;
// // }

// void *AcceptThread(void *arg)
// {
// 	class TcpServer *Tcp = (class TcpServer *)arg;
// 	log_message(INFO, "等待客户端进行连接...");
// 	while (1)
// 	{
// 		if (Tcp->Accept() == false)
// 		{
// 			sleep(1);
// 		}
// 	}
// }

// // void Select_Thread(class TcpServer *Tcp)
// // {
// // 	// class TcpServer *Tcp = (class TcpServer *)arg;
// // 	log_message(INFO, "等待客户端进行连接...");
// // 	int activity = 0;
// // 	// 设置select超时时间
// // 	struct timeval timeout;
// // 	timeout.tv_sec = 0;
// // 	timeout.tv_usec = 10;
// // 	while (1)
// // 	{
// // 		// activity = select(max_sd + 1, &Tcp->readfds, NULL, NULL, &timeout);
// // 	}
// // }
