/**************************************************
	 >Author: zarkdrd
	 >Date: 2024-06-05 17:23:38
     >LastEditTime: 2024-06-21 15:41:21
     >LastEditors: zarkdrd
	 >Description:
     >FilePath: /IMachine_HB/include/CommManage.h
**************************************************/

#ifndef _INCLUDE_COMMMANAGE_H_
#define _INCLUDE_COMMMANAGE_H_

#include "Communication.h"

class CommManage
{
public:
	CommManage();
	~CommManage();

	bool CloseComm(void);
	Communication *CreatComm(int nType, string Dev, int Port);

public:
	static CommManage &GetInstance();
};

#endif //_INCLUDE_COMMMANAGE_H_
