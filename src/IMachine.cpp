/**************************************************
	 >Author: zarkdrd
	 >Date: 2024-07-04 15:03:53
	 >LastEditTime: 2024-08-09 11:33:58
	 >LastEditors: zarkdrd
	 >Description:
	 >FilePath: /IMachine_HB_F_1/src/IMachine.cpp
**************************************************/
#include "tts.h"
#include "convert.h"
#include "GetTime.h"
#include "IMachine.h"
#include "Log_Message.h"

// 定义全局变量
std::atomic<bool> resetTimer(false);
std::mutex displayMutex;

IMachine::IMachine() {

};

IMachine::~IMachine()
{

	delete Tcp1;
	delete Tcp2;
	delete Serial1;
	delete Balustrade_up;
	delete Balustrade_down;
	delete Led;
	delete TrafficLed;
	delete FlashLight;
	delete VehicleDevice1;
	delete VehicleDevice2;
	delete VehicleDevice3;
}

/**************************************初始化区*****************************************/
bool IMachine::ReadCfg(const string &fname)
{
	try
	{
		log_message(INFO, "获取配置文件信息...");

		I1 = IniFile::getInstance()->GetIntValue("IOCardIn", "VehicleDevice1", 1);
		I2 = IniFile::getInstance()->GetIntValue("IOCardIn", "VehicleDevice2", 2);
		I3 = IniFile::getInstance()->GetIntValue("IOCardIn", "VehicleDevice3", 3);
		I4 = IniFile::getInstance()->GetIntValue("IOCardIn", "ManualSwitch", 4);

		TcpPort1 = IniFile::getInstance()->GetIntValue("socket", "RobortPort", 5000);
		TcpPort2 = IniFile::getInstance()->GetIntValue("socket", "ManualPort", 5001);

		O1 = IniFile::getInstance()->GetIntValue("IOCardOut", "Balustrade_up", 1);
		O2 = IniFile::getInstance()->GetIntValue("IOCardOut", "Balustrade_down", 2);
		O3 = IniFile::getInstance()->GetIntValue("IOCardOut", "TrafficLed", 1);
		O4 = IniFile::getInstance()->GetIntValue("IOCardOut", "FlashLight", 2);

		VehicleDevice1 = new Gpio(I1, IN);
		VehicleDevice2 = new Gpio(I2, IN);
		VehicleDevice3 = new Gpio(I3, IN);
		ManualSwitch = new Gpio(I4, IN);

		Balustrade_up = new Gpio(O1, OUT);
		Balustrade_down = new Gpio(O2, OUT);
		TrafficLed = new Gpio(O3, OUT);

		Dev = IniFile::getInstance()->GetStringValue("Serial", "Serial", "/dev/ttyS0");
		port = IniFile::getInstance()->GetIntValue("Serial", "baudRate", 9600);
		Uart1 = IniFile::getInstance()->GetStringValue("Serial", "Led", "/dev/ttyS5");

		communication = IniFile::getInstance()->GetIntValue("Serial", "communication", 0);
		LedMode = IniFile::getInstance()->GetIntValue("Serial", "LedMode", 0);

		Client1 = IniFile::getInstance()->GetStringValue("Clients", "Client1", "192.168.1.2");
		Client2 = IniFile::getInstance()->GetStringValue("Clients", "Client2", "192.168.1.3");
	}
	catch (...)
	{
		log_message(ERROR, "未知错误");
		return false;
	}
	// 通信方式
	if (communication == 0)
	{
		Serial1 = new SerialPort(Dev, port);
	}
	else if (communication == 2)
	{
		Tcp1 = new TcpServer(TcpPort1);
		Tcp2 = new TcpServer(TcpPort2);
	}
	else
	{
		log_message(ERROR, "不存在该通信方式\n");
		return false;
	}

	Led = new QLED;
	TrafficLed = new Gpio(O3, OUT);
	FlashLight = new Gpio(O4, OUT);

	log_message(INFO, "读取配置文件成功\n");

	return true;
}

bool IMachine::Init()
{
	log_message(INFO, "初始化设备\n");
	if (Led->FB_Open(LedMode, Uart1.c_str()) == false)
	{
		return false;
	}
	// Led->DisplayClear(0);
	displayingDefaultContent = false; // 初始化时不显示默认内容
	if (Tcpserver1.joinable() == false)
	{
		Tcpserver1 = std::thread(TcpserverFunc1, this);
	}
	if (Tcpserver2.joinable() == false)
	{
		Tcpserver2 = std::thread(TcpserverFunc2, this);
	}
	if (timerThread.joinable() == false)
	{
		timerThread = std::thread(TimerThread, this);
	}
	if (vehicleThread.joinable() == false)
	{
		vehicleThread = std::thread(VehicleThread, this);
	}

	if (heartThread.joinable() == false)
	{
		heartThread = std::thread(HeartThread, this);
	}
	log_message(INFO, "初始化设备完成...\n");
	return true;
}

bool IMachine::Start()
{
	if (communication == 0)
	{
		while (1)
		{
			if (Serial1->Open() == false)
			{
				usleep(100 * 1000);
				continue;
			}
			ReceiveCmd1();
		}
	}
	else if (communication == 2)
	{
		// if (Tcp1->Open() == false && Tcp2->Open() == false)
		// {
		// 	return false;
		// }
		Tcp1->Open();
		Tcp2->Open();
		while (1)
		{
			if (Tcp1->isOpen == false && Tcp2->isOpen == false)
			{
				usleep(100 * 1000);
				continue;
			}
			// ReceiveCmd1();
			// ReceiveCmd2();
		}
	}
	return false;
}

/**************************************线程区*****************************************/
// 服务器线程
void TcpserverFunc1(class IMachine *myIMachine)
{
	while (1)
	{
		if (myIMachine->Tcp1->isOpen == false)
		{
			usleep(100 * 1000);
			continue;
		}
		myIMachine->ReceiveCmd1();
	}
}

void TcpserverFunc2(class IMachine *myIMachine)
{
	while (1)
	{
		if (myIMachine->Tcp2->isOpen == false)
		{
			usleep(100 * 1000);
			continue;
		}
		myIMachine->ReceiveCmd2();
	}
}

// 电动栏杆机线程
void Balustrade_Thread(class IMachine *myIMachine)
{
	sleep(1);
	myIMachine->Balustrade_up->SetOff();
	myIMachine->Balustrade_down->SetOff();
}
// 更新播放时间
void IMachine::UpdateTimer()
{
	std::lock_guard<std::mutex> lock(displayMutex);	 // 确保线程安全
	lastPlayTime = std::chrono::steady_clock::now(); // 更新最后一次播放指令的时间
	resetTimer = true;								 // 重置计时器标志
}

// 播放默认内容计时器线程
void TimerThread(class IMachine *myIMachine)
{
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 每100毫秒检查一次

		auto now = std::chrono::steady_clock::now();
		auto durationSinceLastPlay = std::chrono::duration_cast<std::chrono::seconds>(now - myIMachine->lastPlayTime);

		if (durationSinceLastPlay.count() >= 10)
		{
			std::lock_guard<std::mutex> lock(myIMachine->displayMutex); // 锁定 displayMutex，确保对显示内容的访问是线程安全的
			if (!myIMachine->displayingDefaultContent)
			{
				unsigned char defaultContent1[] = {0x20, 0x45, 0x54, 0x43, 0x2F, 0xC8, 0xCB, 0xB9, 0xA4};		// 空格起到清屏效果
				unsigned char defaultContent2[] = {0x20, 0xBB, 0xEC, 0xBA, 0xCF, 0xB3, 0xB5, 0xB5, 0xC0, 0x20}; // 显示ETC/人工混合车道
				unsigned char defaultContent3[] = {0xD2, 0xBB, 0xB3, 0xB5, 0xD2, 0xBB, 0xC2, 0xE4, 0xB8, 0xCB}; // 显示一车一落杆
				unsigned char defaultContent4[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}; // 空格起到清屏效果
				int defaultContentLen1 = sizeof(defaultContent1);
				int defaultContentLen2 = sizeof(defaultContent2);
				int defaultContentLen3 = sizeof(defaultContent3);
				int defaultContentLen4 = sizeof(defaultContent4);
				myIMachine->Led->DisplyShow(1, 0, 0x01, 0x09, defaultContent1, defaultContentLen1);
				usleep(450 * 1000);
				myIMachine->Led->DisplyShow(2, 0, 0x01, 0x09, defaultContent2, defaultContentLen2);
				usleep(450 * 1000);
				myIMachine->Led->DisplyShow(3, 0, 0x01, 0x09, defaultContent3, defaultContentLen3);
				usleep(450 * 1000);
				myIMachine->Led->DisplyShow(4, 0, 0x01, 0x09, defaultContent4, defaultContentLen4);
				usleep(450 * 1000);
				myIMachine->displayingDefaultContent = true;
			}
		}
	}
}
// 车道地感线圈设备线程
void VehicleThread(class IMachine *myIMachine)
{
	int i;
	int newIOStatus[9] = {0};
	int oldIOStatus[9] = {0};
	int manualSwitchStatus = 0;
	int ManualSwitchStatusOld = 0;
	unsigned char Oput[3] = {0};
	unsigned int TxSEQ = 0;

	Oput[0] = 0xDC;
	Oput[1] = 0x00; // 默认数据状态正确

	while (1)
	{
		if (myIMachine->Tcp1->isOpen == true || myIMachine->Tcp2->isOpen == true)
		{
			newIOStatus[1] = myIMachine->VehicleDevice1->GetIOValue();
			newIOStatus[2] = myIMachine->VehicleDevice2->GetIOValue();
			newIOStatus[3] = myIMachine->VehicleDevice3->GetIOValue();
			manualSwitchStatus = myIMachine->ManualSwitch->GetIOValue();
			myIMachine->IO4_manualSwitchStatus = manualSwitchStatus;

			for (i = 1; i < 4; i++)
			{
				if ((newIOStatus[i] != -1) && (newIOStatus[i] != oldIOStatus[i]))
				{
					// printf("现在的线圈是%d\n", i);
					// printf("现在的IO是%d\n", newIOStatus[i]);
					if (newIOStatus[3] == 0 && newIOStatus[2] == 0 && newIOStatus[1] == 0)
					{
						Oput[2] = 0x00;
					}
					else if (newIOStatus[3] == 0 && newIOStatus[2] == 0 && newIOStatus[1] == 1)
					{
						Oput[2] = 0x01;
					}
					else if (newIOStatus[3] == 0 && newIOStatus[2] == 1 && newIOStatus[1] == 0)
					{
						Oput[2] = 0x02;
					}
					else if (newIOStatus[3] == 0 && newIOStatus[2] == 1 && newIOStatus[1] == 1)
					{
						Oput[2] = 0x03;
					}
					else if (newIOStatus[3] == 1 && newIOStatus[2] == 0 && newIOStatus[1] == 0)
					{
						Oput[2] = 0x04;
					}
					else if (newIOStatus[3] == 1 && newIOStatus[2] == 0 && newIOStatus[1] == 1)
					{
						Oput[2] = 0x05;
					}
					else if (newIOStatus[3] == 1 && newIOStatus[2] == 1 && newIOStatus[1] == 0)
					{
						Oput[2] = 0x06;
					}
					else if (newIOStatus[3] == 1 && newIOStatus[2] == 1 && newIOStatus[1] == 1)
					{
						Oput[2] = 0x07;
					}
					oldIOStatus[i] = newIOStatus[i];
					TxSEQ = ((TxSEQ / 10) == 0X08) ? 0X10 : (TxSEQ + 10);

					if (manualSwitchStatus == 0)
					{
						myIMachine->CommunicationSend(TxSEQ, 0XDC, &Oput[0], 3, 1, myIMachine->Client1); // 低电平机器人
					}
					else if (manualSwitchStatus == 1)
					{
						myIMachine->CommunicationSend(TxSEQ, 0XDC, &Oput[0], 3, 1, myIMachine->Client2); // 高电平人工
					}
				}
				else if (manualSwitchStatus != ManualSwitchStatusOld)
				{ // 切换后向切换后的发送最新状态
					ManualSwitchStatusOld = manualSwitchStatus;
					if (manualSwitchStatus == 0)
					{
						myIMachine->CommunicationSend(TxSEQ, 0XDC, &Oput[0], 3, 1, myIMachine->Client1);
					}
					else if (manualSwitchStatus == 1)
					{
						myIMachine->CommunicationSend(TxSEQ, 0XDC, &Oput[0], 3, 1, myIMachine->Client2);
					}
				}
			}
		}
		else
		{
			for (int i = 0; i < 8; i++)
			{
				oldIOStatus[i] = -1;
			}
		}
		usleep(10 * 1000);
	}
}

// 心跳线程
void HeartThread(class IMachine *myIMachine)
{
	unsigned int TxSEQ = 0x90;

	while (1)
	{
		if (myIMachine->Tcp1->isOpen == false && myIMachine->Tcp2->isOpen == false)
		{
			sleep(1);
			continue;
		}
		myIMachine->CommunicationSend(TxSEQ, 0xB0, NULL, 0, 0, myIMachine->Client1);
		myIMachine->CommunicationSend(TxSEQ, 0xB0, NULL, 0, 0, myIMachine->Client2);

		sleep(3);
	}
}

/**************************************指令帧区*****************************************/
// 参数配置帧
int IMachine::Configration(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen)
{
	if (Cmd[1] == 0x00)
	{
		log_message(INFO, "正在查询参数................................");
	}
	else if (Cmd[1] == 0x01)
	{
		log_message(INFO, "正在修改参数................................");
	}
	else
	{
		log_message(WARN, "错误的配置帧！");
	}
	return 0;
}
// 电动栏杆机控制帧
int IMachine::BalusTrade(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen)
{
	Oput[0] = 0xD3;
	if (balustradeThread.joinable())
	{
		balustradeThread.join();
	}

	hex_message(INFO, "电动栏杆机指令帧: ", Cmd, CmdLen);
	memcpy(Oput, Cmd, 1);
	if (Cmd[1] == 0x00)
	{
		Balustrade_up->SetOff();
		Balustrade_down->SetOn();
		balustradeThread = std::thread(Balustrade_Thread, this);
		log_message(INFO, "栏杆机: 落杆");
		Oput[1] = 0x03;
	}
	else if (Cmd[1] == 0x01)
	{
		Balustrade_up->SetOn();
		Balustrade_down->SetOff();
		balustradeThread = std::thread(Balustrade_Thread, this);
		log_message(INFO, "栏杆机: 抬杆");
		Oput[1] = 0x02;
	}

	return 4;
}
// 语音播放
int IMachine::DisplayVoice(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen)
{
	Oput[0] = 0xD4;
	hex_message(INFO, "语音播放: ", Cmd, CmdLen);
	int SoundLevel = Cmd[0];

	char VoiceData[256] = {0};
	for (int i = 2; i < CmdLen; i++)
	{
		switch (Cmd[i])
		{
		case 0x00:
			strcat(VoiceData, "0");
			break;
		case 0x01:
			strcat(VoiceData, "1");
			break;
		case 0x02:
			strcat(VoiceData, "2");
			break;
		case 0x03:
			strcat(VoiceData, "3");
			break;
		case 0x04:
			strcat(VoiceData, "4");
			break;
		case 0x05:
			strcat(VoiceData, "5");
			break;
		case 0x06:
			strcat(VoiceData, "6");
			break;
		case 0x07:
			strcat(VoiceData, "7");
			break;
		case 0x08:
			strcat(VoiceData, "8");
			break;
		case 0x09:
			strcat(VoiceData, "9");
			break;
		case 0x0A:
			strcat(VoiceData, "元");
			break;
		case 0x0B:
			strcat(VoiceData, "零");
			break;
		case 0x0C:
			strcat(VoiceData, "十");
			break;
		case 0x0D:
			strcat(VoiceData, "百");
			break;
		case 0x0E:
			strcat(VoiceData, "千");
			break;
		case 0x0F:
			strcat(VoiceData, "万");
			break;
		case 0x10:
			strcat(VoiceData, "一类车");
			break;
		case 0x11:
			strcat(VoiceData, "二类车");
			break;
		case 0x12:
			strcat(VoiceData, "三类车");
			break;
		case 0x13:
			strcat(VoiceData, "四类车");
			break;
		case 0x14:
			strcat(VoiceData, "五类车");
			break;
		case 0x15:
			strcat(VoiceData, "六类车");
			break;
		case 0x16:
			strcat(VoiceData, "七类车");
			break;
		case 0x17:
			strcat(VoiceData, "八类车");
			break;
		case 0x18:
			strcat(VoiceData, "九类车");
			break;
		case 0x19:
			strcat(VoiceData, "金额");
			break;
		case 0x1A:
			strcat(VoiceData, "余额");
			break;
		case 0x1B:
			strcat(VoiceData, "型");
			break;
		case 0x1C:
			strcat(VoiceData, "车");
			break;
		case 0x1D:
			strcat(VoiceData, "道");
			break;
		case 0x1E:
			strcat(VoiceData, "超限率");
			break;
		case 0x1F:
			strcat(VoiceData, "超限重");
			break;
		case 0x20:
			strcat(VoiceData, "总轴重");
			break;
		case 0x21:
			strcat(VoiceData, "吨");
			break;
		case 0x22:
			strcat(VoiceData, "您好");
			break;
		case 0x23:
			strcat(VoiceData, "欢迎光临");
			break;
		case 0x24:
			strcat(VoiceData, "谢谢");
			break;
		case 0x25:
			strcat(VoiceData, "一路平安");
			break;
		case 0x26:
			strcat(VoiceData, "再见");
			break;
		case 0x27:
			strcat(VoiceData, "报警音");
			break;
		case 0x28:
			strcat(VoiceData, "谢谢合作");
			break;
		case 0x29:
			strcat(VoiceData, "正常");
			break;
		case 0x2A:
			strcat(VoiceData, "预付");
			break;
		case 0x2B:
			strcat(VoiceData, "欠款");
			break;
		case 0x2C:
			strcat(VoiceData, "公务");
			break;
		case 0x2D:
			strcat(VoiceData, "军警");
			break;
		case 0x2E:
			strcat(VoiceData, "紧急");
			break;
		case 0x2F:
			strcat(VoiceData, "免费");
			break;
		case 0x30:
			strcat(VoiceData, "车队");
			break;
		case 0x31:
			strcat(VoiceData, "变档");
			break;
		case 0x32:
			strcat(VoiceData, "无卡");
			break;
		case 0x33:
			strcat(VoiceData, "右转");
			break;
		case 0x34:
			strcat(VoiceData, "超时");
			break;
		case 0x35:
			strcat(VoiceData, "违章");
			break;
		case 0x36:
			strcat(VoiceData, "黑车");
			break;
		case 0x37:
			strcat(VoiceData, "黑卡");
			break;
		case 0x38:
			strcat(VoiceData, "坏卡");
			break;
		case 0x39:
			strcat(VoiceData, "更改");
			break;
		case 0x3A:
			strcat(VoiceData, "车牌不符");
			break;
		case 0x3B:
			strcat(VoiceData, "无");
			break;
		case 0x3C:
			strcat(VoiceData, "点");
			break;
		case 0x3D:
			strcat(VoiceData, "系统正在加点自检");
			break;
		case 0x3E:
			strcat(VoiceData, "货车");
			break;
		case 0x3F:
			strcat(VoiceData, "客车");
			break;
		case 0x40:
			strcat(VoiceData, "型车");
			break;
		case 0x41:
			strcat(VoiceData, "农用车");
			break;
		case 0x42:
			strcat(VoiceData, "百分之");
			break;
		case 0x43:
			strcat(VoiceData, "集装箱车");
			break;
		case 0x44:
			strcat(VoiceData, "月票车");
			break;
		case 0x45:
			strcat(VoiceData, ".(点)");
			break;
		case 0x46:
			strcat(VoiceData, "ICC出错");
			break;
		case 0x47:
			strcat(VoiceData, "OBU被锁住");
			break;
		case 0x48:
			strcat(VoiceData, "电子标签被非法拆卸");
			break;
		case 0x49:
			strcat(VoiceData, "电子标签信息错误");
			break;
		case 0x4A:
			strcat(VoiceData, "非本省电子标签");
			break;
		case 0x4B:
			strcat(VoiceData, "非联网电子电子标签");
			break;
		case 0x4C:
			strcat(VoiceData, "电子标签还未启用");
			break;
		case 0x4D:
			strcat(VoiceData, "电子标签卡已经过期");
			break;
		case 0x4E:
			strcat(VoiceData, "该电子标签已经进入黑名单，无效！");
			break;
		case 0x4F:
			strcat(VoiceData, "OBU内无车牌信息,请走普通车道");
			break;
		case 0x50:
			strcat(VoiceData, "OBU信息非法,请走普通车道");
			break;
		case 0x51:
			strcat(VoiceData, "电子标签黑名单");
			break;
		case 0x52:
			strcat(VoiceData, "电子支付卡异常");
			break;
		case 0x53:
			strcat(VoiceData, "非联网电子支付卡");
			break;
		case 0x54:
			strcat(VoiceData, "无湖北省入口信息，请走普通车道");
			break;
		case 0x55:
			strcat(VoiceData, "电子支付卡信息错误");
			break;
		case 0x56:
			strcat(VoiceData, "该电子用户卡已进入黑名单,无效");
			break;
		case 0x57:
			strcat(VoiceData, "余额不足，请走普通车道");
			break;
		case 0x58:
			strcat(VoiceData, "电子支付卡还未启用");
			break;
		case 0x59:
			strcat(VoiceData, "电子支付卡已经过期");
			break;
		case 0x5A:
			strcat(VoiceData, "余额为负,禁止通行");
			break;
		case 0x5B:
			strcat(VoiceData, "卡黑名单");
			break;
		case 0x5C:
			strcat(VoiceData, "其他异常");
			break;
		case 0x5D:
			strcat(VoiceData, "IC卡不存在");
			break;
		case 0x5E:
			strcat(VoiceData, "2-5点大型客车禁止通行!");
			break;
		case 0x5F:
			strcat(VoiceData, "OBU与CPU卡非同省发行");
			break;
		case 0x60:
			strcat(VoiceData, "换卡，请走普通车道");
			break;
		case 0x61:
			strcat(VoiceData, "超时，请走普通车道");
			break;
		case 0x62:
			strcat(VoiceData, "Ｕ行，请走普通车道");
			break;
		case 0x63:
			strcat(VoiceData, "未知缴费方式！");
			break;
		case 0x64:
			strcat(VoiceData, "费率计算失败，请走普通车道！");
			break;
		case 0x65:
			strcat(VoiceData, "入口信息为空或者已交易完毕");
			break;
		case 0x66:
			strcat(VoiceData, "天线未收到标签扣款回复");
			break;
		default:
			break;
		}
	}

	bool success = false; // 设置一个检查成功执行的标志
	try
	{
		TTS::getInstance()->speech_playback(TYPE_UTF8, SoundLevel, VoiceData, strlen(VoiceData));
		success = true; // 如果成功执行，将success标志设置为true
	}
	catch (...)
	{
		log_message(ERROR, "费显语音控制执行失败");
	}
	if (success)
	{
		Oput[1] = 0x04; // 语音播报成功
	}
	else
	{
		Oput[1] = 0x05; // 语音播报失败
	}
	return 4;
}

// 费额显示
int IMachine::DisplayControl(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen)
{
	{
		std::lock_guard<std::mutex> lock(displayMutex);	 // 加锁
		displayingDefaultContent = false;				 // 标记不再显示默认内容
		resetTimer = true;								 // 重置定时器标志
		lastPlayTime = std::chrono::steady_clock::now(); // 更新最后播放时间
		hex_message(INFO, "费额显示: ", Cmd, CmdLen);
		int Font;
		int Brightness;		  // 控制亮度
		int Colours[5] = {0}; // 存储每行的颜色，最多4行
		Oput[0] = 0xD4;

		if (Cmd[0] == 0x44) // 显示屏播放模式，‘D’-显示3行6列 ACSII码为44，‘d’-显示4行8列 ACSII码为64
		{
			Font = 1;
		}
		else if (Cmd[0] == 0x64)
		{
			Font = 0;
		}
		else
		{
			log_message(WARN, "错误的播放模式");
			return -1; // 如果播放模式错误，提前返回
		}
		Brightness = Cmd[1];

		int lineCount = (Cmd[0] == 0x44) ? 3 : 4;	   // 根据模式确定行数
		int charsPerLine = (Cmd[0] == 0x44) ? 12 : 16; // 根据模式确定每行字符数

		// 提取每行的颜色信息
		for (int i = 0; i < lineCount; i++)
		{
			switch (Cmd[3 + i])
			{
			case 0x00: // 红色
				Colours[i + 1] = 0x00;
				break;
			case 0x01: // 绿色
				Colours[i + 1] = 0x01;
				break;
			case 0x02: // 黄色
				Colours[i + 1] = 0x02;
				break;
			default:
				log_message(WARN, "费显设置了第 %d 行为其他颜色", i + 1);
				Colours[i + 1] = Cmd[3 + i];
				break;
			}
		}

		int start = (Cmd[0] == 0x44) ? 6 : 7; // 开始播放位置
		int Line = 1;						  // 播放的行数，1开始
		bool success = false;				  // 设置一个检查成功执行的标志
		try
		{
			for (int i = start; i < CmdLen - start; i += charsPerLine)
			{
				int remainingChars = CmdLen - i;
				int lineLength = (remainingChars > charsPerLine) ? charsPerLine : remainingChars;

				if (Line > lineCount)
					break; // 达到最大行数则退出

				Led->DisplyShow(Line, Font, Colours[Line], 0x09, &Cmd[i], lineLength);
				Line++;
				usleep(450 * 1000);
			}

			Led->DisplayBright(0x02, Brightness);
		}
		catch (...)
		{
			log_message(ERROR, "费显显示控制执行失败");
		}
		if (success)
		{
			Oput[1] = 0x02; // 费显显示正常
		}
		else
		{
			Oput[1] = 0x03; // 费显显示异常
		}
	}
	return 4;
}

// 通行灯指令帧
int IMachine::TrafficControl(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen)
{
	Oput[0] = 0xDE;
	hex_message(INFO, "通行灯指令帧: ", Cmd, CmdLen);
	if (Cmd[1] == 0x01)
	{
		TrafficLed->SetOff();
		log_message(INFO, "通行灯: 红灯");
		Oput[1] = 0x01;
	}
	else if (Cmd[1] == 0x00)
	{
		TrafficLed->SetOn();
		log_message(INFO, "通行灯: 绿灯");
		Oput[1] = 0x00;
	}

	return 4;
}

// 黄闪指令帧
int IMachine::FlashLightControl(unsigned char *Cmd, int CmdLen, unsigned char *Oput, int OputLen)
{
	Oput[0] = 0xDF;
	hex_message(INFO, "黄闪控制指令帧: ", Cmd, CmdLen);
	if (Cmd[0] == 0x01)
	{
		FlashLight->SetOn();
		log_message(INFO, "黄闪: 报警");
		Oput[1] = 0x03;
	}
	else
	{
		FlashLight->SetOff();
		log_message(INFO, "黄闪: 静默");
		Oput[1] = 0x04;
	}

	return 4;
}

// 设备控制帧
int IMachine::Analysis(unsigned char *Cmd, int CmdLen, unsigned char &Type, unsigned char *Oput)
{
	printf("CmdLen: %d\n", CmdLen);
	if (CmdLen < 1)
	{
		log_message(ERROR, "控制指令帧无内容");
		return 0;
	}
	if (Cmd[0] == 0xA0 && Cmd[1] == 0xB0 && Cmd[2] == 0xC0 && Cmd[3] == 0xD0)
	{
		switch (Cmd[4])
		{
		case 0x07: // 黄闪
			if (BCCDataCheck(Cmd, 0, CmdLen, 0))
			{
				log_message(INFO, "黄闪");
				FlashLightControl(&Cmd[4], 8, &Oput[0], 2);
			}
			else
			{
				log_message(WARN, "二次校验失败");
			}
			break;
		case 0x53: // 语音播报
			if (BCCDataCheck(Cmd, 0, CmdLen, 0))
			{
				log_message(INFO, "语音播报");
				DisplayVoice(&Cmd[4], CmdLen - 7, &Oput[0], 2);
			}
			else
			{
				log_message(WARN, "二次校验失败");
			}
			break;
		case 0x44: // 费额显示屏显示3行6列
			if (BCCDataCheck(Cmd, 0, CmdLen, 0))
			{
				log_message(INFO, "费额显示器显示3行6列");
				DisplayControl(&Cmd[4], 42, &Oput[0], 2);
			}
			else
			{
				log_message(WARN, "二次校验失败");
			}
			break;
		case 0x64: // 费额显示屏显示4行8列
			if (BCCDataCheck(Cmd, 0, CmdLen, 0))
			{
				log_message(INFO, "费额显示器显示4行8列");
				DisplayControl(&Cmd[4], 71, &Oput[0], 2);
			}
			else
			{
				log_message(WARN, "二次校验失败");
			}
			break;
		default:
			hex_message(WARN, "费显不支持此设备", &Cmd[4], CmdLen - 1);
			return 0;
		}
	}
	else if (Cmd[0] == 0xA3)
	{
		log_message(INFO, "栏杆机控制");
		BalusTrade(&Cmd[0], 2, &Oput[0], 2);
	}
	else if (Cmd[0] == 0xAE)
	{
		log_message(INFO, "通信灯控制");
		TrafficControl(&Cmd[0], 2, &Oput[0], 2);
	}
	else
	{
		log_message(WARN, "不支持此设备");
	}
	resetTimer = true; // 重置定时器标志
	return 4;
}

/**************************************数据解析区*****************************************/
// 解析数据帧类型
void IMachine::CommunicationAnalysis(unsigned char *Cmd, int CmdLen, string clientIP)
{
	int ResultLen = 2;
	unsigned char Type;
	unsigned char Result[512] = {0};
	unsigned char TxSEQ = Cmd[3];

	if (Cmd[8] == 0xA4)
	{
		// 这里费现请求的处理
		if ((clientIP == Client1 && IO4_manualSwitchStatus == 0) || (clientIP == Client2 && IO4_manualSwitchStatus == 1))
		{
			if (clientIP == Client1 && IO4_manualSwitchStatus == 0)
			{
				log_message(INFO, "机器人模式正确，处理费现控制帧");
			}
			else if (clientIP == Client2 && IO4_manualSwitchStatus == 1)
			{
				log_message(INFO, "人工模式正确，处理费现控制帧");
			}
			switch (Cmd[13])
			{
			case 0x44:
				hex_message(INFO, "收到显示3行6列控制帧:", Cmd, CmdLen);
				Type = 0xD4;
				Analysis(&Cmd[9], CmdLen - 10, Type, Result);
				break;
			case 0x64:
				hex_message(INFO, "收到显示4行8列控制帧:", Cmd, CmdLen);
				Type = 0xD4;
				Analysis(&Cmd[9], CmdLen - 10, Type, Result);
				break;
			case 0x07:
				hex_message(INFO, "收到黄闪控制帧:", Cmd, CmdLen);
				Type = 0xDF;
				Analysis(&Cmd[9], CmdLen - 10, Type, Result);
				break;
			case 0x53:
				hex_message(INFO, "收到费显语音播报控制帧:", Cmd, CmdLen);
				Type = 0xD4;
				Analysis(&Cmd[9], CmdLen - 10, Type, Result);
				break;
			default:
				hex_message(ERROR, "不支持此数据帧:", Cmd, CmdLen);
				return;
			}
		}
		if (clientIP == Client1)
		{
			CommunicationSend(TxSEQ, Type, Result, 2, 0, Client1);
		}
		else if (clientIP == Client2)
		{
			CommunicationSend(TxSEQ, Type, Result, 2, 0, Client2);
		}
	}
	else if (Cmd[8] == 0xA3)
	{
		hex_message(INFO, "收到栏杆机控制帧：", Cmd, CmdLen);
		Type = 0xD3;
		if ((clientIP == Client1 && IO4_manualSwitchStatus == 0) || (clientIP == Client2 && IO4_manualSwitchStatus == 1))
		{
			if (clientIP == Client1 && IO4_manualSwitchStatus == 0)
			{
				log_message(INFO, "机器人模式正确，处理栏杆机控制帧");
			}
			else if (clientIP == Client2 && IO4_manualSwitchStatus == 1)
			{
				log_message(INFO, "人工模式正确，处理栏杆机控制帧");
			}
			Analysis(&Cmd[8], CmdLen, Type, Result);
		}

		if (clientIP == Client1)
		{
			CommunicationSend(TxSEQ, Type, Result, 2, 0, Client1);
		}
		else if (clientIP == Client2)
		{
			CommunicationSend(TxSEQ, Type, Result, 2, 0, Client2);
		}
	}
	else if (Cmd[8] == 0xAE)
	{
		hex_message(INFO, "收到通信灯控制帧：", Cmd, CmdLen);
		Type = 0xDE;
		if ((clientIP == Client1 && IO4_manualSwitchStatus == 0) || (clientIP == Client2 && IO4_manualSwitchStatus == 1))
		{
			if (clientIP == Client1 && IO4_manualSwitchStatus == 0)
			{
				log_message(INFO, "机器人模式正确，处理通信灯控制帧");
			}
			else if (clientIP == Client2 && IO4_manualSwitchStatus == 1)
			{
				log_message(INFO, "人工模式正确，处理通信灯控制帧");
			}
			Analysis(&Cmd[8], CmdLen, Type, Result);
		}

		if (clientIP == Client1)
		{
			CommunicationSend(TxSEQ, Type, Result, 2, 0, Client1);
		}
		else if (clientIP == Client2)
		{
			CommunicationSend(TxSEQ, Type, Result, 2, 0, Client2);
		}
	}
	else if (Cmd[8] == 0xAC)
	{
		hex_message(INFO, "收到地感数据帧：", Cmd, CmdLen);
	}
	else if (Cmd[3] == 0x90)
	{
		hex_message(INFO, "收到车道软件心跳帧：", Cmd, CmdLen);
	}
	else
	{
		hex_message(WARN, "无法识别的指令：", Cmd, CmdLen);
	}
}
// 响应数据发送
bool IMachine::CommunicationSend(unsigned char TxSEQ, unsigned char Type, unsigned char *Cmd, int CmdLen, int isCoil, string clientIP)
{
	int DataLen = 0;
	unsigned char SendData[1024];
	unsigned int BCC = 0;
	SendData[DataLen++] = 0xFF;
	SendData[DataLen++] = 0xFF;
	SendData[DataLen++] = 0x00;
	if (Type == 0xB0)
	{
		SendData[DataLen++] = 0x09;
		SendData[DataLen++] = 0x00;
		SendData[DataLen++] = 0x00;
		SendData[DataLen++] = 0x00;
		SendData[DataLen++] = 0x01;
		SendData[DataLen++] = 0xB0;
	}
	else
	{
		SendData[DataLen++] = TxSEQ / 10;
		SendData[DataLen++] = 0x00;
		SendData[DataLen++] = 0x00;
		SendData[DataLen++] = (CmdLen) / 256;
		SendData[DataLen++] = (CmdLen) % 256;
	}
	if (Type != 0xB0)
	{
		if (Type == 0xDC)
		{
			memcpy(&SendData[DataLen], Cmd, 3);
			DataLen += CmdLen;
		}
		else
		{
			memcpy(&SendData[DataLen], Cmd, 2);
			DataLen += CmdLen;
		}
	}

	BCC = BCCDataCheck(SendData, 2, DataLen - 2, 2);
	SendData[DataLen++] = BCC;

	if (isCoil == 1)
	{
		// 发送地感线圈的响应帧
		if (clientIP == Client1)
		{
			log_message(INFO, "给机器人客户端发送了地感状态。");
			Tcp1->WriteByte(SendData, DataLen);
		}
		else if (clientIP == Client2)
		{
			log_message(INFO, "给人工客户端发送了地感状态。");
			Tcp2->WriteByte(SendData, DataLen);
		}
		else
		{
			log_message(WARN, "Invalid client IP!");
			return false;
		}
	}
	else
	{
		// 发送一般的响应帧
		if (clientIP == Client1)
		{
			if (Tcp1->WriteByte(SendData, DataLen) == false)
			{
				log_message(WARN, "机器人客户端未连接");
			}
		}
		else if (clientIP == Client2)
		{
			if (Tcp2->WriteByte(SendData, DataLen) == false)
			{
				log_message(WARN, "人工客户端未连接");
			}
		}
	}

	switch (SendData[8])
	{
	case 0xB0:
		// hex_message(INFO, "上传心跳帧：", SendData, DataLen);
		break;
	case 0xD0:
		hex_message(INFO, "上传串口参数配置信息:", SendData, DataLen);
		break;
	case 0xDE:
		hex_message(INFO, "上传费显通信灯状态:", SendData, DataLen);
		break;
	case 0xDF:
		hex_message(INFO, "上传费显黄闪状态:", SendData, DataLen);
		break;
	case 0xD3:
		hex_message(INFO, "上传栏杆机状态:", SendData, DataLen);
		break;
	case 0xD4:
		hex_message(INFO, "上传费显显示状态:", SendData, DataLen);
		break;
	case 0xDC:
		hex_message(INFO, "上传地感状态：", SendData, DataLen);
	}
}

// 解析数据头与异或校验
// bool IMachine::ReceiveCmd()
// {
// 	unsigned char Recv_Data_Min1[2048];
// 	unsigned char Recv_Data_Min2[2048];

// 	while (1)
// 	{
// 		int ret = 0;
// 		int ret1 = 0;
// 		int ret2 = 0;
// 		if (communication == 0)
// 		{
// 			ret = Serial1->Recv(Recv_Data_Min1, sizeof(Recv_Data_Min1));
// 			if (ret <= 0)
// 			{
// 				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", ret, strerror(errno));
// 				log_message(ERROR, "串口断开连接，等待重连...");
// 				Serial1->Close();
// 				return false;
// 			}
// 		}
// 		else if (communication == 2)
// 		{
// 			ret1 = Tcp1->RecvByte(Recv_Data_Min1, sizeof(Recv_Data_Min1));
// 			hex_message(INFO, "接收到的数据是：", (unsigned char *)Recv_Data_Min1, ret1);
// 			handleReceiveCmd(ret1, Recv_Data_Min1);
// 			if (ret1 <= 0)
// 			{
// 				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", ret1, strerror(errno));
// 				log_message(ERROR, "客户端断开连接，等待重连...");
// 				Tcp1->Close();
// 				return false;
// 			}
// 			ret2 = Tcp2->RecvByte(Recv_Data_Min2, sizeof(Recv_Data_Min2));
// 			hex_message(INFO, "接收到的数据是：", (unsigned char *)Recv_Data_Min2, ret2);
// 			handleReceiveCmd(ret2, Recv_Data_Min2);
// 			if (ret2 <= 0)
// 			{
// 				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", ret2, strerror(errno));
// 				log_message(ERROR, "客户端断开连接，等待重连...");
// 				Tcp2->Close();
// 				return false;
// 			}
// 		}
// 		else
// 		{
// 			return false;
// 		}
// 	}
// }

// bool IMachine::handleReceiveCmd(int ret, unsigned char *Recv_Data_Min)
// {
// 	int i;
// 	int Index1 = -1, Index2 = 0, pos = 0;
// 	int Command_Len, Frame_Data_Len; // 这里用Command_Len表示不同类型数据帧的长度，Frame_Data_Len表示接收到的数据帧长度
// 	unsigned char Recv_Data[2048];
// 	unsigned char Recv_Data_bak[2048];
// 	while (true)
// 	{
// 		/**将获取到的数据进行保存**/
// 		memcpy(Recv_Data + pos, Recv_Data_Min, ret);
// 		pos += ret;
// 		if (pos < 4)
// 		{
// 			continue;
// 		}
// 	Find_Frame:
// 		for (i = Index2; i < pos - 1; i++)
// 		{
// 			/**找到数据头**/
// 			if ((Recv_Data[i] == 0xFF) && (Recv_Data[i + 1] == 0xFF) && (Recv_Data[i + 2] == 0x00))
// 			{
// 				Index1 = i;
// 				break;
// 			}
// 		}
// 		if (Index1 < 0)
// 		{
// 			// cout << "现在的数据：" << Recv_Data[Index1] << endl;
// 			/**未找到数据头，清空数据**/
// 			Index2 = 0;
// 			pos = 0;
// 			continue;
// 		}
// 		if (pos - Index1 < 7)
// 		{
// 			continue;
// 		}
// 		else
// 		{
// 			/**获取指令长度**/
// 			if (Recv_Data[i + 13] == 0x44)
// 			{
// 				Command_Len = 58;
// 			}
// 			else if (Recv_Data[i + 13] == 0x64)
// 			{
// 				Command_Len = 87;
// 			}
// 			else if (Recv_Data[i + 13] == 0x07)
// 			{
// 				Command_Len = 23;
// 			}
// 			else if (Recv_Data[i + 8] == 0xAE)
// 			{
// 				Command_Len = 11;
// 			}
// 			else if (Recv_Data[i + 13] == 0x53)
// 			{
// 				Command_Len = Recv_Data[i + 6] + 9 + 10; // 除去语音部分还需加上集成化数据帧的部分
// 			}
// 			else if (Recv_Data[i + 8] == 0xA3)
// 			{
// 				Command_Len = 11;
// 			}
// 			else if (Recv_Data[i + 3] == 0x90) // 车道软件发过来的心跳帧长度
// 			{
// 				Command_Len = 9;
// 			}
// 		}
// 		if (Command_Len > (pos - Index1))
// 		{
// 			/**数据帧不完整**/
// 			continue;
// 		}
// 		else if (Command_Len < (pos - Index1))
// 		{
// 			/**大于一帧数据**/
// 			Frame_Data_Len = Command_Len;
// 			Index2 = Index1 + Frame_Data_Len;
// 		}
// 		else
// 		{
// 			/**刚好一帧数据**/
// 			Index2 = 0;
// 			Frame_Data_Len = Command_Len;
// 		}
// 		// printf("Index2 = %d\n", Index2);
// 		// 调用 BCC 校验函数
// 		if (BCCDataCheck(Recv_Data, Index1 + 2, Frame_Data_Len - 2, 0) == 1)
// 		{
// 			log_message(INFO, "异或计算成功");
// 			CommunicationAnalysis(&Recv_Data[Index1], Frame_Data_Len);
// 		}
// 		else
// 		{
// 			/**说明这个数据帧是错的，找第二个头**/
// 			// hex_message(ERROR, "Recv Tcp data err:", &Recv_Data[Index1], Frame_Data_Len);
// 			for (i = Index1 + 1; i < pos - 1; ++i)
// 			{
// 				/**找到数据头**/
// 				if (Recv_Data[i] == 0xFF && Recv_Data[i + 1] == 0xFF)
// 				{
// 					Index1 = -1;
// 					Index2 = i;
// 					goto Find_Frame;
// 				}
// 			}
// 			Index2 = 0;
// 			pos = 0;
// 			continue;
// 		}
// 		if (Index2 == 0)
// 		{
// 			/**无拼帧情况**/
// 			pos = 0;
// 			Index1 = -1;
// 			Index2 = 0;
// 			memset(Recv_Data, 0, sizeof(Recv_Data));
// 			continue;
// 		}
// 		if (Index2 > 0)
// 		{
// 			/**有拼帧情况**/
// 			log_message(INFO, "处理了拼接数据帧里的一条指令");
// 			pos = pos - Index2;
// 			memcpy(Recv_Data_bak, Recv_Data + Index2, pos);
// 			memset(Recv_Data, 0, sizeof(Recv_Data));
// 			memcpy(Recv_Data, Recv_Data_bak, pos);
// 		}

// 		if (pos < 4)
// 		{
// 			/**不够完整的帧**/
// 			Index1 = -1;
// 			Index2 = 0;
// 			continue;
// 		}
// 		if (Recv_Data[9] == 0xA0 && Recv_Data[10] == 0xB0 && Recv_Data[11] == 0xC0 && Recv_Data[12] == 0xD0)
// 		{
// 			if ((Recv_Data[7] + 9) > pos)
// 			{
// 				/**帧长度不够**/
// 				Index1 = -1;
// 				Index2 = 0;
// 				continue;
// 			}
// 		}
// 		else if (Recv_Data[3] == 0x90)
// 		{
// 			if (Recv_Data[5] + 7 > pos)
// 			{
// 				/**帧长度不够**/
// 				Index1 = -1;
// 				Index2 = 0;
// 				continue;
// 			}
// 		}
// 		else
// 		{
// 			if ((Recv_Data[7]) > pos)
// 			{
// 				/**帧长度不够**/
// 				Index1 = -1;
// 				Index2 = 0;
// 				continue;
// 			}
// 		}
// 		/**帧长度充足，再次解析帧**/
// 		Index1 = -1;
// 		Index2 = 0;
// 		goto Find_Frame;
// 	}
// }
bool IMachine::ReceiveCmd1()
{
	int i;
	int ret, Index1 = -1, Index2 = 0, pos = 0;
	int Command_Len, Frame_Data_Len; // 这里用Command_Len表示不同类型数据帧的长度，Frame_Data_Len表示接收到的数据帧长度
	unsigned char Recv_Data_Min[2048];
	unsigned char Recv_Data[2048];
	unsigned char Recv_Data_bak[2048];

	while (1)
	{
		if (communication == 0)
		{
			ret = Serial1->Recv(Recv_Data_Min, sizeof(Recv_Data_Min));
			if (ret <= 0)
			{
				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", ret, strerror(errno));
				log_message(ERROR, "串口断开连接，等待重连...");
				Serial1->Close();
				return false;
			}
		}
		else if (communication == 2)
		{
			ret = Tcp1->RecvByte(Recv_Data_Min, sizeof(Recv_Data_Min));
			if (ret <= 0)
			{
				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", ret, strerror(errno));
				log_message(ERROR, "机器人客户端断开连接，等待重连...");
				Tcp1->Close();
				return false;
			}
		}
		else
		{
			return false;
		}
		log_message(INFO, "5000端接收到[%s]客户端的数据长度[%d]", Client2, ret);
		hex_message(INFO, "接收内容Recv_Data_Min为：", Recv_Data_Min, ret);
		/**将获取到的数据进行保存**/
		memcpy(Recv_Data + pos, Recv_Data_Min, ret);

		pos += ret;
		hex_message(INFO, "Recv_Data为：", Recv_Data, pos);

		if (pos < 4)
		{
			continue;
		}
	Find_Frame:
		for (i = Index2; i < pos - 1; i++)
		{
			/**找到数据头**/
			if ((Recv_Data[i] == 0xFF) && (Recv_Data[i + 1] == 0xFF))
			{
				Index1 = i;
				break;
			}
		}
		if (Index1 < 0)
		{
			// cout << "现在的数据：" << Recv_Data[Index1] << endl;
			/**未找到数据头，清空数据**/
			Index2 = 0;
			pos = 0;
			continue;
		}
		if (pos - Index1 < 7)
		{
			continue;
		}
		else
		{
			/**获取指令长度**/
			if (Recv_Data[i + 13] == 0x44)
			{
				Command_Len = 58;
			}
			else if (Recv_Data[i + 13] == 0x64)
			{
				Command_Len = 87;
			}
			else if (Recv_Data[i + 13] == 0x07)
			{
				Command_Len = 23;
			}
			else if (Recv_Data[i + 8] == 0xAE)
			{
				Command_Len = 11;
			}
			else if (Recv_Data[i + 13] == 0x53)
			{
				Command_Len = Recv_Data[i + 6] + 9 + 10; // 除去语音部分还需加上集成化数据帧的部分
			}
			else if (Recv_Data[i + 8] == 0xA3)
			{
				Command_Len = 11;
			}
			else if (Recv_Data[i + 3] == 0x90) // 车道软件发过来的心跳帧长度
			{
				Command_Len = 9;
			}
		}
		if (Command_Len > (pos - Index1))
		{
			/**数据帧不完整**/
			continue;
		}
		else if (Command_Len < (pos - Index1))
		{
			/**大于一帧数据**/
			Frame_Data_Len = Command_Len;
			Index2 = Index1 + Frame_Data_Len;
		}
		else
		{
			/**刚好一帧数据**/
			Index2 = 0;
			Frame_Data_Len = Command_Len;
		}
		// printf("Index2 = %d\n", Index2);
		// 调用 BCC 校验函数
		if (BCCDataCheck(Recv_Data, Index1 + 2, Frame_Data_Len - 2, 0) == 1)
		{
			log_message(INFO, "异或计算成功");
			string str1 = Client1;
			CommunicationAnalysis(&Recv_Data[Index1], Frame_Data_Len, str1);
		}
		else
		{
			/**说明这个数据帧是错的，找第二个头**/
			// hex_message(ERROR, "Recv Tcp data err:", &Recv_Data[Index1], Frame_Data_Len);
			for (i = Index1 + 1; i < pos - 1; ++i)
			{
				/**找到数据头**/
				if (Recv_Data[i] == 0xFF && Recv_Data[i + 1] == 0xFF)
				{
					Index1 = -1;
					Index2 = i;
					goto Find_Frame;
				}
			}
			Index2 = 0;
			pos = 0;
			continue;
		}
		if (Index2 == 0)
		{
			/**无拼帧情况**/
			pos = 0;
			Index1 = -1;
			Index2 = 0;
			memset(Recv_Data, 0, sizeof(Recv_Data));
			continue;
		}
		if (Index2 > 0)
		{
			/**有拼帧情况**/
			pos = pos - Index2;
			memcpy(Recv_Data_bak, Recv_Data + Index2, pos);
			memset(Recv_Data, 0, sizeof(Recv_Data));
			memcpy(Recv_Data, Recv_Data_bak, pos);
		}

		if (pos < 4)
		{
			/**不够完整的帧**/
			Index1 = -1;
			Index2 = 0;
			continue;
		}
		if (Recv_Data[9] == 0xA0 && Recv_Data[10] == 0xB0 && Recv_Data[11] == 0xC0 && Recv_Data[12] == 0xD0)
		{
			if ((Recv_Data[7] + 9) > pos)
			{
				/**帧长度不够**/
				Index1 = -1;
				Index2 = 0;
				continue;
			}
		}
		else if (Recv_Data[3] == 0x90)
		{
			if (Recv_Data[5] + 7 > pos)
			{
				/**帧长度不够**/
				Index1 = -1;
				Index2 = 0;
				continue;
			}
		}
		else
		{
			if ((Recv_Data[7]) > pos)
			{
				/**帧长度不够**/
				Index1 = -1;
				Index2 = 0;
				continue;
			}
		}
		/**帧长度充足，再次解析帧**/
		Index1 = -1;
		Index2 = 0;
		goto Find_Frame;
	}
}
bool IMachine::ReceiveCmd2()
{
	int i;
	int ret, Index1 = -1, Index2 = 0, pos = 0;
	int Command_Len, Frame_Data_Len; // 这里用Command_Len表示不同类型数据帧的长度，Frame_Data_Len表示接收到的数据帧长度
	unsigned char Recv_Data_Min[2048];
	unsigned char Recv_Data[2048];
	unsigned char Recv_Data_bak[2048];

	while (1)
	{
		if (communication == 0)
		{
			ret = Serial1->Recv(Recv_Data_Min, sizeof(Recv_Data_Min));
			if (ret <= 0)
			{
				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", ret, strerror(errno));
				log_message(ERROR, "串口断开连接，等待重连...");
				Serial1->Close();
				return false;
			}
		}
		else if (communication == 2)
		{
			ret = Tcp2->RecvByte(Recv_Data_Min, sizeof(Recv_Data_Min));
			if (ret <= 0)
			{
				log_message(ERROR, "recv 返回值 [%d] 错误描述 [%s]", ret, strerror(errno));
				log_message(ERROR, "人工客户端断开连接，等待重连...");
				Tcp2->Close();
				return false;
			}
		}
		else
		{
			return false;
		}
		log_message(INFO, "5001端接收到[%s]客户端的数据", Client2);
		hex_message(INFO, "接收内容Recv_Data_Min为：", Recv_Data_Min, ret);
		/**将获取到的数据进行保存**/
		memcpy(Recv_Data + pos, Recv_Data_Min, ret);
		pos += ret;
		hex_message(INFO, "Recv_Data为：", Recv_Data_Min, ret);
		if (pos < 4)
		{
			continue;
		}
	Find_Frame:
		for (i = Index2; i < pos - 1; i++)
		{
			/**找到数据头**/
			if ((Recv_Data[i] == 0xFF) && (Recv_Data[i + 1] == 0xFF))
			{
				Index1 = i;
				break;
			}
		}
		if (Index1 < 0)
		{
			// cout << "现在的数据：" << Recv_Data[Index1] << endl;
			/**未找到数据头，清空数据**/
			Index2 = 0;
			pos = 0;
			continue;
		}
		if (pos - Index1 < 7)
		{
			continue;
		}
		else
		{
			/**获取指令长度**/
			if (Recv_Data[i + 13] == 0x44)
			{
				Command_Len = 58;
			}
			else if (Recv_Data[i + 13] == 0x64)
			{
				Command_Len = 87;
			}
			else if (Recv_Data[i + 13] == 0x07)
			{
				Command_Len = 23;
			}
			else if (Recv_Data[i + 8] == 0xAE)
			{
				Command_Len = 11;
			}
			else if (Recv_Data[i + 13] == 0x53)
			{
				Command_Len = Recv_Data[i + 6] + 9 + 10; // 除去语音部分还需加上集成化数据帧的部分
			}
			else if (Recv_Data[i + 8] == 0xA3)
			{
				Command_Len = 11;
			}
			else if (Recv_Data[i + 3] == 0x90) // 车道软件发过来的心跳帧长度
			{
				Command_Len = 9;
			}
		}
		if (Command_Len > (pos - Index1))
		{
			/**数据帧不完整**/
			continue;
		}
		else if (Command_Len < (pos - Index1))
		{
			/**大于一帧数据**/
			Frame_Data_Len = Command_Len;
			Index2 = Index1 + Frame_Data_Len;
		}
		else
		{
			/**刚好一帧数据**/
			Index2 = 0;
			Frame_Data_Len = Command_Len;
		}
		// printf("Index2 = %d\n", Index2);
		// 调用 BCC 校验函数
		if (BCCDataCheck(Recv_Data, Index1 + 2, Frame_Data_Len - 2, 0) == 1)
		{
			log_message(INFO, "异或计算成功");
			string str = Client2;
			CommunicationAnalysis(&Recv_Data[Index1], Frame_Data_Len, str);
		}
		else
		{
			/**说明这个数据帧是错的，找第二个头**/
			// hex_message(ERROR, "Recv Tcp data err:", &Recv_Data[Index1], Frame_Data_Len);
			for (i = Index1 + 1; i < pos - 1; ++i)
			{
				/**找到数据头**/
				if (Recv_Data[i] == 0xFF && Recv_Data[i + 1] == 0xFF)
				{
					Index1 = -1;
					Index2 = i;
					goto Find_Frame;
				}
			}
			Index2 = 0;
			pos = 0;
			continue;
		}
		if (Index2 == 0)
		{
			/**无拼帧情况**/
			pos = 0;
			Index1 = -1;
			Index2 = 0;
			memset(Recv_Data, 0, sizeof(Recv_Data));
			continue;
		}
		if (Index2 > 0)
		{
			/**有拼帧情况**/
			pos = pos - Index2;
			memcpy(Recv_Data_bak, Recv_Data + Index2, pos);
			memset(Recv_Data, 0, sizeof(Recv_Data));
			memcpy(Recv_Data, Recv_Data_bak, pos);
		}

		if (pos < 4)
		{
			/**不够完整的帧**/
			Index1 = -1;
			Index2 = 0;
			continue;
		}
		if (Recv_Data[9] == 0xA0 && Recv_Data[10] == 0xB0 && Recv_Data[11] == 0xC0 && Recv_Data[12] == 0xD0)
		{
			if ((Recv_Data[7] + 9) > pos)
			{
				/**帧长度不够**/
				Index1 = -1;
				Index2 = 0;
				continue;
			}
		}
		else if (Recv_Data[3] == 0x90)
		{
			if (Recv_Data[5] + 7 > pos)
			{
				/**帧长度不够**/
				Index1 = -1;
				Index2 = 0;
				continue;
			}
		}
		else
		{
			if ((Recv_Data[7]) > pos)
			{
				/**帧长度不够**/
				Index1 = -1;
				Index2 = 0;
				continue;
			}
		}
		/**帧长度充足，再次解析帧**/
		Index1 = -1;
		Index2 = 0;
		goto Find_Frame;
	}
}

// BCC校验
unsigned int IMachine::BCCDataCheck(unsigned char *data, int startIndex, int length, int type)
{
	unsigned int xor_checksum = 0;
	int num = 0;
	if (type == 0)
	{
		for (int i = startIndex; i < startIndex + length - 1; ++i)
		{
			xor_checksum ^= data[i];
		}
		if ((xor_checksum & 0xFF) == data[startIndex + length - 1])
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		for (int i = startIndex; i < startIndex + length; i++)
		{
			xor_checksum ^= data[i];
		}
		return xor_checksum & 0xFF;
	}
}