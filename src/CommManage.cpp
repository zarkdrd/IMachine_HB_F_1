/**************************************************
	 >Author: zarkdrd
	 >Date: 2024-06-05 17:22:59
     >LastEditTime: 2024-06-21 15:47:11
     >LastEditors: zarkdrd
	 >Description:
     >FilePath: /IMachine_HB/src/CommManage.cpp
**************************************************/

#include "Tcp.h"
#include "Udp.h"
#include "Uart.h"
#include "CommManage.h"
#include "Communication.h"
#include "Log_Message.h"

CommManage::CommManage()
{
}

CommManage::~CommManage()
{
}

Communication *CommManage::CreatComm(int nType, string Dev, int Port)
{
	Communication *comm;
	if (nType == 0)
	{
		comm = new SerialPort(Dev, Port);
	}
	else
	{
		comm = new TcpClient(Dev, Port);
	}

	if (comm->Open() == false)
	{
		log_message(ERROR, "打开设备 [%s : %d] 失败", Dev, Port);
		delete comm;
		return NULL;
	}
	return comm;
}

bool CommManage::CloseComm(void)
{
	return true;
}

CommManage &CommManage::GetInstance()
{
	static CommManage instance;
	return instance;
}
