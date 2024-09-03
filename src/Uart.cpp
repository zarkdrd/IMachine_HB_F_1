/*************************************************************************
    > File Name: src/SerialPort.cpp
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-10-30
 ************************************************************************/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#include "Uart.h"
#include "Log_Message.h"

SerialPort::SerialPort(string port, int baudRate)
{
	isOpen = false;
	_port = port;
	_baudRate = SetBaud(baudRate);
}

SerialPort::~SerialPort()
{
	Close();
}

speed_t SerialPort::SetBaud(int baudRate)
{
	switch(baudRate){
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
			log_message(INFO, "不存在 [B%d] 波特率选项, 默认使用 [B115200]", baudRate);
			return B115200;
	}
}

bool SerialPort::Open()
{
	if(isOpen){
		Close();
	}
	fd = open(_port.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0){
        log_message(ERROR, "Open [%s] : %s", _port.c_str(), strerror(errno));
        return false;
    }

    struct termios  myios;
    memset(&myios, 0, sizeof(myios));

    cfsetispeed(&myios, _baudRate);
    cfsetospeed(&myios, _baudRate);

    myios.c_cflag |= CLOCAL;   /**Ignore serial port control line**/
    myios.c_cflag |= CREAD;    /**Set receive flag**/
    myios.c_cflag &= ~CRTSCTS; /**Do not use flow control**/
    myios.c_cflag &= ~CSIZE;; /**Shield other data bits**/
    myios.c_cflag |= CS8;      /**Set 8 data bits**/
    myios.c_cflag &= ~PARENB;  /**Set no parity bit**/
    myios.c_cflag &= ~CSTOPB;  /**Set 1 stop bit**/
    myios.c_oflag &= ~OPOST;   /**No processing, raw data output**/

    myios.c_cc[VTIME] = 0;
    myios.c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &myios) < 0) {
        log_message(ERROR, "Open [%s] tcsetattr : %s", _port.c_str(), strerror(errno));
        return false;
    }

    isOpen = true;

	return true;
}

bool SerialPort::Close()
{
	close(fd);
	isOpen = false;
	return true;
}

int SerialPort::Send(void* buffer, int len)
{
	if(!isOpen){
		log_message(INFO, "串口 [%s] 设备异常或者未打开", _port.c_str());
		return false;
	}

	if(write(fd, buffer, len) < 0){
		log_message(ERROR, "写串口失败: %s", strerror(errno));
		return false;
	}

	return true;
}

int SerialPort::Recv(void* buffer, int len)
{
	int RecvLen = read(fd, buffer, len);
	if(RecvLen < 0){
		log_message(ERROR, "read err: %s", strerror(errno));
	}
	return RecvLen;
}

int SerialPort::Recv(void* buffer, int len, int sec, int msec)
{
	if(!isOpen){
		log_message(INFO, "串口 [%s] 设备异常或者未打开", _port.c_str());
		return false;
	}
	fd_set rfds;
	struct timeval tv;
	tv.tv_sec = sec;
	tv.tv_usec = msec * 1000;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	int retval = select(fd + 1, &rfds, NULL, NULL, &tv);
	if (retval < 0) {
		log_message(ERROR, "select : %s", strerror(errno));
		return -1;
	}else if (retval == 0) {
		return 0;
	}

	return read(fd, buffer, len);
}

bool SerialPort::Clear()
{
	tcflush(fd, TCIOFLUSH);
	return true;
}

int  SerialPort::GetHandleCode()
{
	return fd;
}

bool SerialPort::GetStatus()
{
	return isOpen;
}
