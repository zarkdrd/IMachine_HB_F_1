/*************************************************************************
    > File Name: src/GetTime.cpp
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-01
 ************************************************************************/

#include <stdio.h>
#include "GetTime.h"

double Run_Timecnt(struct timeval *t_start, struct timeval *t_end)
{
	long int time_consumed_sec = 0;//单位秒
	long int time_consumed_usec = 0;//单位微秒
	double time_consumed;//单位毫秒

	time_consumed_sec  = t_end->tv_sec  - t_start->tv_sec;
	time_consumed_usec = t_end->tv_usec - t_start->tv_usec;
	time_consumed = (double)((time_consumed_sec * 1000) + (time_consumed_usec * 0.001));

	return time_consumed;
}

double Run_SysTimecnt(struct timespec *t_start, struct timespec *t_end)
{
	long int time_consumed_sec = 0;//单位秒
	long int time_consumed_nsec = 0;//单位纳秒
	double time_consumed;//单位毫秒
	time_consumed_sec = t_end->tv_sec - t_start->tv_sec;
	time_consumed_nsec = t_end->tv_nsec - t_start->tv_nsec;
	time_consumed = (double)((time_consumed_sec * 1000) + (time_consumed_nsec * 0.000001));

	return time_consumed;
}
void GetUnixTime(unsigned char *localTime)
{
	time_t now = time(NULL);
	localTime[0] = (now >> 24) & 0XFF;
	localTime[1] = (now >> 16) & 0XFF;
	localTime[2] = (now >> 8) & 0XFF;
	localTime[3] = (now >> 0) & 0XFF;
}

void GetUnixTime8(unsigned char *localTime)
{
	time_t now = time(NULL);
    localTime[0] = (now >> 28) & 0xFF; // 获取高位字节
    localTime[1] = (now >> 24) & 0xFF;
    localTime[2] = (now >> 20) & 0xFF;
    localTime[3] = (now >> 16) & 0xFF;
    localTime[4] = (now >> 12) & 0xFF;
    localTime[5] = (now >> 8) & 0xFF;
    localTime[6] = (now >> 4) & 0xFF;
    localTime[7] = now & 0xFF; // 获取低位字节
}