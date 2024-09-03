/**************************************************
     >Author: zarkdrd
     >Date: 2024-06-04 14:24:38
     >LastEditTime: 2024-06-06 09:48:38
     >LastEditors: zarkdrd
     >Description: 
     >FilePath: /IMachine_HB/include/QYLED.h
**************************************************/


#ifndef _INCLUDE_QYLED_H_
#define _INCLUDE_QYLED_H_

#include "kernel_list.h"
#include "Communication.h"

class QLED{
	public:
		QLED();
		~QLED();

		bool FB_Open(int nType, const char* sParas);
		bool DisplayInit(int nType, const char* Dev, int Port);
		bool DisplayReply(unsigned int timeout);
		bool DisplayPage(int Page); //切换显示页
		bool DisplaySetRelay(int bit, int Value, int sec); //设置继电器
		bool DisplaySwitch(int Status);	//软开关
		bool DisplayBright(int Mode, int Size); //亮度控制
		bool DisplayFast(int Line, int Font, int Colour, const void *Data, int Data_Len);
		bool DisplaySpecially(int Line, int Font, int Colour, int Specially, const void *Data, int Data_Len);
		bool DisplyShow(int Line, int Font, int Colour, int Specially, const void *Data, int Data_Len);
		bool DisplayClear(int Line);
		bool DisplayExit();

		bool TrafficVoiceInit(int nType, const char* Dev, int Port);
		bool VoicePlay(int Mode, void *Data, int Data_Len);
		bool TrafficLed(int Line, int Mode);
	public:
		string QDEV;
		int DisplayMode;
		class Klist *ledList;
		class Communication *comm;
	private:
		int oldpage;
		pthread_t DisplyID;
};

void *DisplayThread(void *arg);

#endif //_INCLUDE_QYLED_H_
