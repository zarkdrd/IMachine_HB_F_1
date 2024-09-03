/**************************************************
	 >Author: zarkdrd
	 >Date: 2024-06-04 15:56:29
	 >LastEditTime: 2024-08-07 17:35:17
	 >LastEditors: zarkdrd
	 >Description:
	 >FilePath: /IMachine_HB_F/include/IMachine.h
**************************************************/

#ifndef _INCLUDE_IMACHINE_H_
#define _INCLUDE_IMACHINE_H_

#include "QYLED.h"
#include "IODevice.h"
#include "TcpServer.h"
#include "Uart.h"
#include "Get_Inifiles.h"
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

class IMachine
{
public:
	IMachine();
	~IMachine();
	bool ReadCfg(const string &fname);
	bool Init();
	bool Start();
	bool ReceiveCmd1();
	bool ReceiveCmd2();
	bool handleReceiveCmd(int ret, unsigned char *Recv_Data_Min);
	int Analysis(unsigned char *Cmd, int CmdLen, unsigned char &Type, unsigned char *Oput);
	void CommunicationAnalysis(unsigned char *Cmd, int CmdLen,string clientIP);
	bool CommunicationSend(unsigned char TxSEQ, unsigned char Type, unsigned char *Cmd, int CmdLen, int isCoil, string clientIP);
	void UpdateTimer(); // 更新播放时间
	unsigned int BCCDataCheck(unsigned char *data, int startIndex, int length, int type);

public:
	int Configration(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen);		 // 参数配置
	int DeviceInit(unsigned char *Cmd, int CmdLen);											 // 设备初始化
	int BalusTrade(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen);		 // 自动栏杆机
	int DisplayVoice(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen);		 // 语音
	int DisplayControl(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen);	 // 费额显示
	int TrafficControl(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen);	 // 通行信号灯
	int FlashLightControl(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen); // 黄闪指令帧
public:
	class TcpServer *Tcp1, *Tcp2;
	class Gpio *VehicleDevice1, *VehicleDevice2, *ManualSwitch, *VehicleDevice3, *VehicleDevice4, *VehicleDevice5, *VehicleDevice6, *VehicleDevice7, *VehicleDevice8;
	class SerialPort *Serial1;
	class QLED *Led;
	class Gpio *Balustrade_up, *Balustrade_down;
	int communication;
	int LedMode;

public:
	bool displayingDefaultContent;						// 添加一个变量来跟踪是否正在显示默认内容
	std::chrono::steady_clock::time_point lastPlayTime; // 最后播放的时间
	std::atomic<bool> resetTimer;						// 时间置位
	std::mutex displayMutex;							// 播放资源锁

private:
	int TcpPort1;
	int TcpPort2;
	string ntpIP;

public:
	string Client1;
	string Client2;

	int IO4_manualSwitchStatus; // 人工或机器人模式

private:
	string Dev;
	int port;
	string Uart1;
	class Gpio *TrafficLed;
	class Gpio *FlashLight;
	int O1, O2, O3, O4, O5, O6, O7, O8, I1, I2, I3, I4, I5, I6, I7, I8;

private:
	class IniFile *Config;

private:
	std::thread balustradeThread;
	std::thread timerThread;
	std::thread vehicleThread;
	std::thread heartThread;
	std::thread Tcpserver1;
	std::thread Tcpserver2;
};
void TimerThread(class IMachine *myIMachine);
void VehicleThread(class IMachine *myIMachine);
void HeartThread(class IMachine *myIMachine);
void Balustrade_Thread(class IMachine *myIMachine);
void TcpserverFunc1(class IMachine *myIMachine);
void TcpserverFunc2(class IMachine *myIMachine);
#endif //_INCLUDE_IMACHINE_H_
