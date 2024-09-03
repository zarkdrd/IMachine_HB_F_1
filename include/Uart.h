/**************************************************
     >Author: zarkdrd
     >Date: 2024-06-05 17:56:32
     >LastEditTime: 2024-06-05 17:56:50
     >LastEditors: zarkdrd
     >Description: 
     >FilePath: /IMachine_HB/include/Uart.h
**************************************************/


#ifndef _INCLUDE_UART_H_
#define _INCLUDE_UART_H_

#include <iostream>
#include <termios.h>

#include "Communication.h"

class SerialPort : public Communication{
	public:
		SerialPort(string port, int baudRate);
		~SerialPort();

		virtual bool Open(void) override;
		virtual bool Close(void) override;
		virtual int  Send(void* buffer, int len) override;
		virtual int  Recv(void* buffer, int len) override;
		virtual int  Recv(void* buffer, int len, int sec, int msec) override;
		virtual int  GetHandleCode() override;
		virtual bool GetStatus() override;

		bool Clear();
	private:
		speed_t SetBaud(int baudRate);

	private:
		int fd;
		bool isOpen;
		string _port;
		speed_t _baudRate;
};

#endif //_INCLUDE_UART_H_
