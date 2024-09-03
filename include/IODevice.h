/*************************************************************************
    > File Name: include/IODevice.h
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-01
 ************************************************************************/

#ifndef _INCLUDE_IODEVICE_H_
#define _INCLUDE_IODEVICE_H_

#define OUT     "out"
#define IN      "in"
#define HIGH_LEVEL  "1"
#define LOW_LEVEL   "0"

using namespace std;

class Gpio{
	public:
		Gpio(int GpioPin, const char *Mode);
		~Gpio();
		bool SetOpen();
		bool SetClose();
		bool SetOn();
		bool SetOff();
		int  GetIOValue();
		bool Exit();

		bool SetPin(int GpioPin);
	private:
		int fd;
		int Pin;
		string PinName;
		string mode;
	public:
		bool isOpen;
};


#endif //_INCLUDE_IODEVICE_H_
