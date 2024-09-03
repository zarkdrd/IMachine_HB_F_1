/*************************************************************************
    > File Name: include/GetTime.h
    > Author: ARTC
    > Descripttion:
    > Created Time: 2023-11-01
 ************************************************************************/

#ifndef _INCLUDE_GETTIME_H_
#define _INCLUDE_GETTIME_H_

#include <time.h>
#include <sys/time.h>

double Run_Timecnt(struct timeval *t_start, struct timeval *t_end);
double Run_SysTimecnt(struct timespec *t_start, struct timespec *t_end);
void GetUnixTime(unsigned char *localTime);
void GetUnixTime8(unsigned char *localTime);

#endif //_INCLUDE_GETTIME_H_
