/**************************************************
     >Author: zarkdrd
     >Date: 2024-07-04 15:03:53
     >LastEditTime: 2024-07-21 14:14:00
     >LastEditors: zarkdrd
     >Description: 
     >FilePath: /IMachine_HB_B/include/Log_Message.h
**************************************************/
#ifndef __LOG_MESSAGE_H__
#define __LOG_MESSAGE_H__

#ifdef __cplusplus
extern "C"{
#endif

#define _CRT_SECURE_NO_DEPRECATE

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <link.h>
#include <dlfcn.h>

#define ThreadID syscall(__NR_gettid)

#define SLASH "/"
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)

#define  TRACE 0
#define  INFO  1
#define  WARN  2
#define  ERROR 3

void hex_try_append(int lvl, unsigned long PID, const char* head, unsigned char* buffer, int buffer_len);
void log_try_append(int lvl, unsigned long PID, const char* filename, int file_line, const char* func, const char* format, ...);
#define hex_message(type, head, buffer, buffer_len)	hex_try_append(type, ThreadID, head, buffer, buffer_len)
#define log_message(type, fmt, ...) log_try_append(type, ThreadID, __FILE__, __LINE__, __FUNCTION__, fmt,  ##__VA_ARGS__, #__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
