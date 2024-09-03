/*************************************************************************
    > File Name: include/Communication.h
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-25
 ************************************************************************/

#ifndef _INCLUDE_COMMUNICATION_H_
#define _INCLUDE_COMMUNICATION_H_

#include <iostream>
using namespace std;

class Communication{
	public:
		Communication();
		virtual ~Communication();

		virtual bool Open(void) = 0;
		virtual bool Close(void) = 0;
		virtual int  Send(void* buffer, int len) = 0;
		virtual int  Recv(void* buffer, int len) = 0;
		virtual int  Recv(void* buffer, int len, int sec, int msec) = 0;
		virtual int  GetHandleCode() = 0;

		virtual bool GetStatus() = 0;
};

#endif //_INCLUDE_COMMUNICATION_H_
