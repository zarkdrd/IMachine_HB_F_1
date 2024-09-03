/**************************************************
	 >Author: zarkdrd
	 >Date: 2024-06-06 10:05:16
     >LastEditTime: 2024-07-21 14:26:31
     >LastEditors: zarkdrd
	 >Description:
     >FilePath: /IMachine_HB_B/src/main.cpp
**************************************************/

#include <signal.h>
#include <iostream>
#include "IMachine.h"
#include "Log_Message.h"

static void handler(int sig)
{
	if (sig == 2)
	{
		log_message(INFO, "退出当前进程");
	}
	else
	{
		log_message(ERROR, "进程崩溃");
	}

	sleep(2);
	exit(-1);
}
static void Exit(void)
{
	sleep(2);
	exit(0);
}

int main(void)
{
	signal(SIGINT, handler);
	signal(SIGSEGV, handler);

	IniFile::getInstance()->Load("Config.ini");
	class IMachine *myIMachine;
	myIMachine = new IMachine();
	if (myIMachine->ReadCfg("Config.ini") == false)
	{
		delete myIMachine;
		Exit();
	}
	if (myIMachine->Init() == false)
	{
		delete myIMachine;
		Exit();
	}
	myIMachine->Start();

	sleep(2);

	return 0;
}
