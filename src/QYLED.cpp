/**************************************************
	 >Author: zarkdrd
	 >Date: 2024-06-04 14:24:09
     >LastEditTime: 2024-06-14 15:42:59
     >LastEditors: zarkdrd
	 >Description:
     >FilePath: /IMachine_HB/src/QYLED.cpp
**************************************************/

#include <string.h>
#include "QYLED.h"
#include "GetTime.h"
#include "CommManage.h"
#include "Log_Message.h"

#include <iostream>
using namespace std;

QLED::QLED()
{
	comm = NULL;
	ledList = NULL;
	DisplayMode = 1;
	oldpage = -1;
	ledList = new Klist;
}
QLED::~QLED()
{
	DisplayExit();
}

bool QLED::FB_Open(int nType, const char *sParas)
{
	log_message(INFO, "打开费显 nType = [%d] sParas = [%s]", nType, sParas);
	if ((nType != 2) && (nType != 0))
	{
		log_message(ERROR, "不支持使用除网口和串口之外使用其他方式进行通讯");
		return false;
	}
	string strParam = sParas;
	string sIp1 = strParam.substr(0, strParam.find_first_of(','));
	if (sIp1.length() <= 0)
	{
		return false;
	}
	strParam = strParam.substr(sIp1.length() + 1, strParam.length() - sIp1.length() - 1);
	string Port = strParam.substr(0, strParam.find_first_of(','));
	if (Port.length() <= 0)
	{
		log_message(ERROR, "传入参数错误，需要传入设备地址与端口号");
		return false;
	}

	return DisplayInit(nType, sIp1.c_str(), atoi(Port.c_str()));
}

bool QLED::DisplayInit(int nType, const char *Dev, int Port)
{
	QDEV = Dev;
	if (nType == 0)
	{
		comm = CommManage::GetInstance().CreatComm(nType, Dev, Port);
	}
	else
	{
		comm = CommManage::GetInstance().CreatComm(nType, Dev, Port);
	}

	if (comm == NULL)
	{
		log_message(INFO, "打开费显设备失败");
		return false;
	}

	log_message(INFO, "打开费显 [%s : %d] 成功", QDEV.c_str(), Port);
	pthread_create(&DisplyID, NULL, DisplayThread, this);
	usleep(10 * 1000);
	return true;
}

/*
bool QLED::DisplayReply(unsigned int timeout)
{
	int ret = 0, pos = 0;
	unsigned char buf[512] = {0};

	struct timeval t_start, t_end;
	double time_consumed;

	gettimeofday(&t_start, NULL);
	usleep(50 * 1000);
	while(1){
		ret = comm->Recv(buf + pos, 512, 0, 10);
		if(ret < 0){
			log_message(ERROR, "获取费显返回数据失败: %s", strerror(errno));
			return false;
		}else if(ret == 0){
			gettimeofday(&t_end, NULL);
			time_consumed = Run_Timecnt(&t_start, &t_end);
			if(time_consumed > (timeout - 50)){
				log_message(ERROR, "获取费显 [%s] 返回数据超时 %03fms", QDEV.c_str(), time_consumed);
				if(pos > 0){
					hex_message(INFO, "费显返回数据内容为", buf, pos);
				}
				return false;
			}
			continue;
		}

		pos = pos + ret;
		if(pos >= 21){
			switch(buf[17]){
				case 0x31:
					break;
				case 0x32:
					hex_message(WARN, "费显返回接收异常指令", buf, pos);
					return false;
				case 0x33:
					hex_message(WARN, "费显返回要求重传指令", buf, pos);
					return false;
				default:
					hex_message(WARN, "费显返回未识别指令", buf, pos);
					return false;
			}
			break;
		}
	}

	return true;
}
*/
bool QLED::DisplayPage(int Page)
{
	if (oldpage == Page)
	{
		return true;
	}
	oldpage = Page;

	int pos = 0;
	unsigned char status_data[128] = {0};

	status_data[pos++] = 0xFE; // 报文头
	status_data[pos++] = 0x5C;
	status_data[pos++] = 0x4B;
	status_data[pos++] = 0x89;

	status_data[pos++] = 0x15; // 数据包总长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = 0x66; // 显示页切换

	status_data[pos++] = 0x92; // 发送ID
	status_data[pos++] = 0x79;
	status_data[pos++] = 0x95;
	status_data[pos++] = 0x72;

	status_data[pos++] = 0x02; // 具体指令长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = Page;
	status_data[pos++] = 0xFF - Page;

	status_data[pos++] = 0xFF;
	status_data[pos++] = 0xFF;

	ledList->ShowAdd(7, status_data, pos, 150);
	if (Page == 1)
	{
		ledList->ShowAdd(7, status_data, pos, 150);
	}

	return true;
}

bool QLED::DisplaySetRelay(int bit, int Value, int sec)
{
	int pos = 0;
	unsigned char status_data[128] = {0};
	static unsigned char Relay1[2] = {0xFD, 0x02}, Relay2[2] = {0xFD, 0x02}, Relay3[2] = {0xFD, 0x02};

	switch (bit)
	{
	case 0x01:
		if (Value == 0)
		{
			Relay1[0] = 0xFD;
			Relay1[1] = 0x02;
		}
		else if ((Value != 0) && (sec == 0))
		{
			Relay1[0] = 0xFC;
			Relay1[1] = 0x03;
		}
		else
		{
			Relay1[0] = 0xB0 + (sec * 2);
			Relay1[1] = 0xFF - Relay1[0];
		}
		break;
	case 0x02:
		if (Value == 0)
		{
			Relay2[0] = 0xFD;
			Relay2[1] = 0x02;
		}
		else if ((Value != 0) && (sec == 0))
		{
			Relay2[0] = 0xFC;
			Relay2[1] = 0x03;
		}
		else
		{
			Relay2[0] = 0xB0 + (sec * 2);
			Relay2[1] = 0xFF - Relay1[0];
		}
		break;
	case 0x03:
		if (Value == 0)
		{
			Relay3[0] = 0xFD;
			Relay3[1] = 0x02;
		}
		else if ((Value != 0) && (sec == 0))
		{
			Relay3[0] = 0xFC;
			Relay3[1] = 0x03;
		}
		else
		{
			Relay3[0] = 0xB0 + (sec * 2);
			Relay3[1] = 0xFF - Relay1[0];
		}
		break;
	default:
		return false;
	}

	status_data[pos++] = 0xFE; // 报文头
	status_data[pos++] = 0x5C;
	status_data[pos++] = 0x4B;
	status_data[pos++] = 0x89;

	status_data[pos++] = 0x19; // 数据包总长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = 0x61; // 继电器控制

	status_data[pos++] = 0x00; // 发送ID
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = 0x06; // 具体指令长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = Relay1[0];
	status_data[pos++] = Relay1[1];
	status_data[pos++] = Relay2[0];
	status_data[pos++] = Relay2[1];
	status_data[pos++] = Relay3[0];
	status_data[pos++] = Relay3[1];

	status_data[pos++] = 0xFF;
	status_data[pos++] = 0xFF;

	ledList->ShowAdd(8, status_data, pos, 150);
	return true;
}

bool QLED::DisplaySwitch(int Status)
{
	int pos = 0;
	unsigned char status_data[128] = {0};

	status_data[pos++] = 0xFE; // 报文头
	status_data[pos++] = 0x5C;
	status_data[pos++] = 0x4B;
	status_data[pos++] = 0x89;

	status_data[pos++] = 0x13; // 数据包总长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	if (Status)
	{
		status_data[pos++] = 0x51; // 软开屏
	}
	else
	{
		status_data[pos++] = 0x52; // 软关屏
	}

	status_data[pos++] = 0x00; // 消息ID写0即可
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = 0x00; // 具体指令长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = 0xFF;
	status_data[pos++] = 0xFF;

	ledList->ShowAdd(9, status_data, pos, 150);
	return true;
}

bool QLED::DisplayBright(int Mode, int Size)
{
	int pos = 0;
	unsigned char status_data[128] = {0};

	status_data[pos++] = 0xFE; // 报文头
	status_data[pos++] = 0x5C;
	status_data[pos++] = 0x4B;
	status_data[pos++] = 0x89;

	status_data[pos++] = 0x17; // 数据包总长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = 0x76; // 设置亮度

	status_data[pos++] = 0x00; // 消息ID写0即可
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	status_data[pos++] = 0x04; // 具体指令长度
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;
	status_data[pos++] = 0x00;

	if (Mode)
	{
		Mode = 0x01; // 内部自动亮度控制
	}
	else
	{
		Mode = 0x02; // 外部亮度控制
	}
	status_data[pos++] = Mode; // 选择亮度控制方式
	status_data[pos++] = 0xFF - Mode;

	if (Size > 7)
	{
		Size = 0x07;
	}
	status_data[pos++] = 7 - Size; // 选择亮度大小
	status_data[pos++] = 0xFF - (7 - Size);

	status_data[pos++] = 0xFF;
	status_data[pos++] = 0xFF;

	ledList->ShowAdd(10, status_data, pos, 150);

	return 0;
}

bool QLED::DisplayFast(int Line, int Font, int Colour, const void *Data, int Data_Len)
{
	unsigned int pos = 0;
	unsigned int fee_display_data_len = Data_Len + 24; // 数据帧总长度
	unsigned int display_data_len = Data_Len + 5;	   // 数据帧数据域长度
	unsigned char display_data[1024] = {0};

	display_data[pos++] = 0xFE; // 包头
	display_data[pos++] = 0x5C;
	display_data[pos++] = 0x4B;
	display_data[pos++] = 0x89;

	display_data[pos++] = (fee_display_data_len << 0) & 0xFF; // 数据帧数据域大小(不包括这4字节与包尾巴)
	display_data[pos++] = (fee_display_data_len << 8) & 0xFF;
	display_data[pos++] = (fee_display_data_len << 16) & 0xFF;
	display_data[pos++] = (fee_display_data_len << 24) & 0xFF;

	display_data[pos++] = 0x65; // 消息类型(实时采集)

	display_data[pos++] = 0x00; // ID
	display_data[pos++] = 0x00;
	display_data[pos++] = 0x00;
	display_data[pos++] = 0x00;

	display_data[pos++] = (display_data_len << 0) & 0xFF; // 数据帧数据域大小(不包括这4字节与包尾巴)
	display_data[pos++] = (display_data_len << 8) & 0xFF;
	display_data[pos++] = (display_data_len << 16) & 0xFF;
	display_data[pos++] = (display_data_len << 24) & 0xFF;

	display_data[pos++] = Line; // 种类编号
	display_data[pos++] = 0x00; // 闪烁标志，默认不闪烁

	display_data[pos++] = (0x01) | ((Colour & 0x0F) << 4); // 高4位字体颜色

	if (Font > 0x03)
	{
		Font = 0x03; // 字体大于3时，显示不全
	}
	display_data[pos++] = (0x10) | (Font & 0x0F); // 高4位字体,低4位字体大小

	display_data[pos++] = Data_Len;				// 显示内容长度
	memcpy(display_data + pos, Data, Data_Len); // 显示具体内容
	pos = pos + Data_Len;

	display_data[pos++] = 0xFF; // 包尾巴
	display_data[pos] = 0xFF;
	display_data_len = pos + 1;

	ledList->ShowAdd(Line, display_data, display_data_len, 80);

	return 0;
}

bool QLED::DisplaySpecially(int Line, int Font, int Colour, int Specially, const void *Data, int Data_Len)
{
	unsigned int pos = 0;
	unsigned int material_data_len = Data_Len + 10;			   // 素材长度
	unsigned int fee_display_data_len = Data_Len + 84;		   // 具体数据帧长度
	unsigned int display_data_len = fee_display_data_len - 19; // 数据帧数据域长度
	unsigned char display_data[1024] = {0};

	display_data[pos++] = 0xFE; // 包头
	display_data[pos++] = 0x5C;
	display_data[pos++] = 0x4B;
	display_data[pos++] = 0x89;

	display_data[pos++] = (fee_display_data_len >> 0) & 0xFF; // 数据帧总长度
	display_data[pos++] = (fee_display_data_len >> 8) & 0xFF;
	display_data[pos++] = (fee_display_data_len >> 16) & 0xFF;
	display_data[pos++] = (fee_display_data_len >> 24) & 0xFF;

	display_data[pos++] = 0x31; // 消息类型

	display_data[pos++] = 0x00; // ID
	display_data[pos++] = 0x00;
	display_data[pos++] = 0x00;
	display_data[pos++] = 0x00;

	display_data[pos++] = (display_data_len >> 0) & 0xFF; // 数据帧数据域大小(不包括这4字节与包尾巴)
	display_data[pos++] = (display_data_len >> 8) & 0xFF;
	display_data[pos++] = (display_data_len >> 16) & 0xFF;
	display_data[pos++] = (display_data_len >> 24) & 0xFF;

	display_data[pos++] = 0x30; // 素材UID，表示行数
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x30 + Line;

	display_data[pos++] = 0x2C; // 分隔符,

	display_data[pos++] = Specially; // 移动方式(特效) 9:立即显示
	display_data[pos++] = 0x05;		 // 移动速度
	display_data[pos++] = 0x00;		 // 停留时间

	display_data[pos++] = 0x30; // 播放时间段
	display_data[pos++] = 0x31;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x31;
	display_data[pos++] = 0x30;
	display_data[pos++] = 0x31;
	display_data[pos++] = 0x39;
	display_data[pos++] = 0x39;
	display_data[pos++] = 0x31;
	display_data[pos++] = 0x32;
	display_data[pos++] = 0x33;
	display_data[pos++] = 0x31;

	display_data[pos++] = 0x13; // 素材属性长度
	display_data[pos++] = 0x00;
	display_data[pos++] = 0x00;
	display_data[pos++] = 0x00;

	display_data[pos++] = 0x55; // 标志字节
	display_data[pos++] = 0xAA;

	display_data[pos++] = 0x00; // 保留字节
	display_data[pos++] = 0x00;

	display_data[pos++] = 0x37; // 内容属性,37默认内码文字
	display_data[pos++] = 0x32; // 31：掉电保存  32：掉电不保存
	display_data[pos++] = 0x32; // 更新时间 31：所有素材立即更新 32:本素材立即更新 33：本素材稍后更新

	display_data[pos++] = 0x31; // 文本起始标志
	display_data[pos++] = 0x33; // 显示屏颜色单、双、三基色(31~33)
	display_data[pos++] = 0x31; // 图片编码方式
	display_data[pos++] = 0x00; // 保留字节
	display_data[pos++] = 0x00;

	display_data[pos++] = 0x10; // 区域宽度
	display_data[pos++] = 0x00; // 区域宽度
	display_data[pos++] = 0x20; // 区域高度
	display_data[pos++] = 0x00; // 区域高度

	display_data[pos++] = Colour; // 颜色
	if (Font > 0x03)
	{
		Font = 0x03; // 超过限制显示不全
	}
	display_data[pos++] = (0x10 & 0xF0) | (Font & 0x0F); // 字节高表示字体，字节低表示字号

	display_data[pos++] = 0x00; // 保留字节

	display_data[pos++] = (material_data_len >> 0) & 0xFF;	// 素材内容长度(不包括这四个字节)
	display_data[pos++] = (material_data_len >> 8) & 0xFF;	// 素材内容长度
	display_data[pos++] = (material_data_len >> 16) & 0xFF; // 素材内容长度
	display_data[pos++] = (material_data_len >> 24) & 0xFF; // 素材内容长度

	memcpy(display_data + pos, Data, Data_Len); // 显示具体内容
	pos = pos + Data_Len;

	display_data[pos++] = 0xFF; // 素材内容特性字
	display_data[pos++] = 0x00;

	display_data[pos++] = 0x01; // 本次素材内容的序号
	display_data[pos++] = 0x00;

	display_data[pos++] = 0x01; // 固定字节
	display_data[pos++] = 0x00;
	display_data[pos++] = 0x01;
	display_data[pos++] = 0x00;

	display_data[pos++] = 0x10; // 保留字节
	display_data[pos++] = 0x48;

	display_data[pos++] = 0x2D; // 素材内容结束标志
	display_data[pos++] = 0x31;
	display_data[pos++] = 0x2C;

	display_data[pos++] = 0xFF; // 包尾巴
	display_data[pos] = 0xFF;
	display_data_len = pos + 1;

	ledList->ShowAdd(Line, display_data, display_data_len, 450);
	return 0;
}

bool QLED::DisplyShow(int Line, int Font, int Colour, int Specially, const void *Data, int Data_Len)
{
	if (Data_Len > 20)
	{
		Specially = 0x01;
	}
	char buf[128] = {0};
	memcpy(buf, Data, Data_Len);

	log_message(INFO, "费显控制: 第%d行显示内容 [%s]", Line, buf);

	if (DisplayMode == 0)
	{
		return DisplayFast(Line, Font, Colour, Data, Data_Len);
	}
	else
	{
		return DisplaySpecially(Line, Font, Colour, Specially, Data, Data_Len);
	}
}

bool QLED::DisplayClear(int Line)
{
	unsigned char clearbuf[16] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};
	if (Line == 0)
	{
		for (int i = 1; i < 5; i++)
		{
			if (DisplayMode == 0)
			{
				DisplayFast(i, 0, 1, clearbuf, 8);
			}
			else
			{
				DisplaySpecially(i, 0, 1, 9, clearbuf, 8);
			}
		}
		sleep(2);
		log_message(INFO, "显示控制: 清除全屏");
	}
	else
	{
		log_message(INFO, "显示控制: 清除第%d行显示", Line);
		if (DisplayMode == 0)
		{
			return DisplayFast(Line, 0, 1, clearbuf, 2);
		}
		else
		{
			return DisplaySpecially(Line, 0, 1, 9, clearbuf, 2);
		}
	}
	return true;
}

bool QLED::DisplayExit()
{
	if (comm != NULL)
	{
		delete comm;
	}

	if (ledList != NULL)
	{
		delete ledList;
	}
	comm = NULL;
	ledList = NULL;

	return true;
}

bool QLED::VoicePlay(int Mode, void *Data, int Data_Len)
{
	int pos = 0;
	int voice_len = Data_Len + 2;
	int voice_data_len = Data_Len + 26;
	unsigned char voice_data[1024] = {0};

	voice_data[pos++] = 0xFE; // 报文头
	voice_data[pos++] = 0x5C;
	voice_data[pos++] = 0x4B;
	voice_data[pos++] = 0x89;

	voice_data[pos++] = (voice_data_len >> 0) & 0xFF; // 数据包总长度
	voice_data[pos++] = (voice_data_len >> 8) & 0xFF;
	voice_data[pos++] = (voice_data_len >> 16) & 0xFF;
	voice_data[pos++] = (voice_data_len >> 24) & 0xFF;

	voice_data[pos++] = 0x68; // 转发类型
	voice_data[pos++] = 0x02; // 转发端口COM2

	voice_data[pos++] = 0x00; // 发送ID，默认00
	voice_data[pos++] = 0x00;
	voice_data[pos++] = 0x00;

	int len = voice_len + 5;
	voice_data[pos++] = (len >> 0) & 0XFF; // 具体指令长度
	voice_data[pos++] = (len >> 8) & 0XF;
	voice_data[pos++] = (len >> 16) & 0XF;
	voice_data[pos++] = (len >> 24) & 0XF;

	/**串口语音命令指令帧**/
	voice_data[pos++] = 0xFD; // 帧头

	voice_data[pos++] = (voice_len >> 8) & 0xFF; // 数据字符总长度高位
	voice_data[pos++] = voice_len & 0xFF;		 // 命令字+编码+播放内容长度

	voice_data[pos++] = 0x01;				  // 开始语音合成
	voice_data[pos++] = Mode;				  // 语音编码格式 00:GB2312 01:GBK 02:BIG5 03:UNICODE
	memcpy(voice_data + pos, Data, Data_Len); // 语音播放内容

	pos += Data_Len;
	voice_data[pos++] = 0x00; // 保留两个字节
	voice_data[pos++] = 0x00;

	voice_data[pos++] = 0xFF; // 包尾巴
	voice_data[pos++] = 0xFF;

	ledList->ShowAdd(1, voice_data, pos, 150);

	char buf[512] = {0};
	memcpy(buf, Data, Data_Len);
	log_message(INFO, "语音播报: %s", buf);

	return true;
}

bool QLED::TrafficLed(int Line, int Mode)
{
	int pos = 0;
	unsigned char current_data[128] = {0};

	current_data[pos++] = 0xFE; // 报文头
	current_data[pos++] = 0x5C;
	current_data[pos++] = 0x4B;
	current_data[pos++] = 0x89;

	current_data[pos++] = 0x20; // 数据包总长度
	current_data[pos++] = 0x00;
	current_data[pos++] = 0x00;
	current_data[pos++] = 0x00;

	current_data[pos++] = 0x67; // 消息类型固定67

	current_data[pos++] = 0x00; // 消息ID写0即可
	current_data[pos++] = 0x00;
	current_data[pos++] = 0x00;
	current_data[pos++] = 0x00;

	current_data[pos++] = 0x0D; // 具体指令长度
	current_data[pos++] = 0x00;
	current_data[pos++] = 0x00;
	current_data[pos++] = 0x00;

	current_data[pos++] = 0x01; // 数据项数量
	current_data[pos++] = 0xFE; // 前一项的反码

	current_data[pos++] = 0x00; // 0:立即更新 1:稍后更新

	current_data[pos++] = 0x00; // 保留
	current_data[pos++] = 0x00;

	current_data[pos++] = Line; // 区域号

	if (Mode == 0)
	{
		current_data[pos++] = 0x00; // 图片起始序号
		current_data[pos++] = 0x00;
	}
	else
	{
		current_data[pos++] = 0x01; // 图片起始序号
		current_data[pos++] = 0x00;
	}

	current_data[pos++] = 0x01; // 图片数量低位
	current_data[pos++] = 0x00;

	current_data[pos++] = 0x00; // 移动方式
	current_data[pos++] = 0x00;
	current_data[pos++] = 0x00;

	current_data[pos++] = 0xFF;
	current_data[pos++] = 0xFF;

	ledList->ShowAdd(2, current_data, pos, 150);

	return true;
}

void *DisplayThread(void *arg)
{
	Node *node;
	Node_Data data;
	data.status = true;
	int buffLen = 0;
	unsigned int TimeOut;
	unsigned char buff[1024];
	class QLED *led = (class QLED *)arg;
	led->ledList->ListTaskInit();
	while (1)
	{
		node = led->ledList->FindNode(&data);
		if ((node == NULL) || led->comm->GetStatus() == false)
		{
			usleep(10 * 1000);
			continue;
		}

		pthread_mutex_lock(&led->ledList->Mutex);
		node->data.status = false;
		TimeOut = node->data.TimeOut;
		buffLen = node->data.QLED_Data_Len;
		memcpy(buff, node->data.QLED_Data, buffLen);
		pthread_mutex_unlock(&led->ledList->Mutex);

		hex_message(INFO, "写费显", buff, buffLen);
		led->comm->Send(buff, buffLen);
		// if(led->DisplayReply(TimeOut) == true){
		//	continue;
		// }

		led->comm->Send(buff, buffLen);
		// hex_message(INFO, "写费显重传", buff, buffLen);
		// led->DisplayReply(TimeOut);
	}
}
