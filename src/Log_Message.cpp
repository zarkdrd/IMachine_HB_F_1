#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>
#include <mutex>
#include <regex>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <iterator>
#include <iostream>
#include <condition_variable>

#include "dirent.h"
#include "Log_Message.h"
#include "Get_Inifiles.h"

#define LOG_WRITE 1						// 默认写日志
#define LOG_DEBUG 0						// 默认关闭终端调试
#define LOG_LEVEL 1						// 默认日志级别为1
#define LOG_CFG_NAME		"Config.ini"
#define LOG_NAME_HEAD		"Device"	// 默认日志文件名前缀
#define LOG_PATH_DIR_NAME	"ArtcLog"	// 默认日志存放路径
#define LOG_RETENTION_DAYS	7			// 默认保存7天的日志
#define LOG_MINIMUMSPACE	30			// 默认最小剩余空间不足30M时，关闭日志接口

#define LOG_LEN_LIMIT		4096		// 限制一条日志最长不超过4K
#define LOG_USE_LIMIT		(30L * 1024* 1024)      // 默认单个文件最大字节数为30M
#define LOG_BUF_USE_LIMIT	(10L * 1024* 1024)		// 默认单个内存块最大字节数为10M, 超过会直接丢弃

#define LOG_BUFF_WAIT_TIME  1			// 默认1秒写一次日志， 或者内存块满了写日志


#define MAX_PATH_LEN 256

static bool GetModuleFileName(std::string &cfgpath, std::string &path, std::string &name)
{
    int ret;
    int pos = 0;
    char buf[256] = {0};
    void *address = (void*)(&GetModuleFileName);

    Dl_info info;
    info.dli_fname = 0;
    ret = dladdr(address, &info);
    if ((ret != 0) && (info.dli_fname != NULL)) {
        if(strlen(info.dli_fname) >= MAX_PATH_LEN) {
            return false;
        }
        for(int i = 0; i < strlen(info.dli_fname); i++){
            if(info.dli_fname[i] == '/'){
                pos = i;
            }
        }
		if((pos ==0) && (info.dli_fname[0] != '/')){
			path = "";
			cfgpath = LOG_CFG_NAME;
			strcpy(buf, info.dli_fname);
		}else if((pos == 0) && (info.dli_fname[0] == '/')){
			memcpy(buf, info.dli_fname, pos + 1);
			path = std::string(buf);
			cfgpath = path + LOG_CFG_NAME;
			memset(buf, 0, sizeof(buf));
			strcpy(buf, info.dli_fname + pos + 1);
		}else{
			memcpy(buf, info.dli_fname, pos + 1);
			path = std::string(buf);
			cfgpath = path + LOG_CFG_NAME;
			memset(buf, 0, sizeof(buf));
			strcpy(buf, info.dli_fname + pos + 1);
		}

		name = buf;

		if(name.length() == 0){
			return false;
		}
        return true;
    }
    return false;
}

long long get_free_space_mb(const char *path) {
    struct statvfs fs_info;
    long long free_space;

    if (statvfs(path, &fs_info) != 0) {
        return -1;
    }

    free_space = ((long long)fs_info.f_bsize * fs_info.f_bfree) / (1024 * 1024);
    return free_space ;
}

class utc_timer {
public:
	utc_timer() {
		memset(utc_fmt, 0, sizeof(utc_fmt));
		struct timeval tv;
		gettimeofday(&tv, NULL);
		sys_sec = tv.tv_sec;
		sys_min = sys_sec / 60;
		struct tm utc_tm;
		localtime_r(&sys_sec, &utc_tm);
		year = utc_tm.tm_year + 1900;
		mon	 = utc_tm.tm_mon + 1;
		day  = utc_tm.tm_mday;
		hour = utc_tm.tm_hour;
		min  = utc_tm.tm_min;
		sec  = utc_tm.tm_sec;
		msec = tv.tv_usec / 1000;
		reset_utc_fmt();
	}
	std::string get_utc_timer(){
		struct timeval tv;
		gettimeofday(&tv, NULL);
		utc_mtx.lock();
		msec = tv.tv_usec / 1000;
		if (tv.tv_sec != sys_sec){
			sec = tv.tv_sec % 60;
			sys_sec = tv.tv_sec;
			if (sys_sec / 60 != sys_min){
				sys_min = sys_sec / 60;
				struct tm utc_tm;
				localtime_r(&sys_sec, &utc_tm);
				year = utc_tm.tm_year + 1900;
				mon = utc_tm.tm_mon + 1;
				day = utc_tm.tm_mday;
				hour = utc_tm.tm_hour;
				min = utc_tm.tm_min;
			}
		}
		reset_utc_fmt();
		std::string m_utc_fmt = utc_fmt;
		utc_mtx.unlock();
		return m_utc_fmt;
	}
private:
	void reset_utc_fmt(){
		snprintf(utc_fmt, 20, "%02d:%02d:%02d.%03d", hour, min, sec, msec);
	}
public:
	char utc_fmt[32];
	int year, mon, day, hour, min, sec, msec;
private:
	std::mutex utc_mtx;
	time_t sys_sec, sys_min;
};

class MyLogClass{
public:
	MyLogClass();
	static MyLogClass* getInstance(){
		if (m_Instance == NULL) {
			m_Instance = new MyLogClass();
		}
		return m_Instance;
	}
	void log_thread_to_show();
	bool createdirectory(const std::string& dir_path);
	bool RemoveFile(char* file_name, int max_days);
	bool find_directory_to_del_old_files(const char* dir_name, const char* file_name, int max_days);
	bool write_to_file(int _year, int _mon, int _day);
	void try_append(char* log_line, size_t log_len);

private:
	void InitMyLogClass();
public:
	int log_level;
	class utc_timer* _time;
private:
	FILE* log_fp;
	bool isNew;
	int year, mon, day;
	unsigned long count;
	std::string log_path, log_name;
	static class MyLogClass* m_Instance;
	int log_write, log_debug, log_days;
private:
	char log_head_end[LOG_LEN_LIMIT + 3];
	char log_head_line[LOG_LEN_LIMIT + 3];
	char *log_buffer, *log_data;
	size_t log_buffer_len, log_data_len, log_head_line_len, log_head_end_len;

	pthread_t log_thread;
	pthread_cond_t  log_cond;
	pthread_mutex_t log_mutex;
};
MyLogClass* MyLogClass::m_Instance = NULL;

void *log_tread_func(void* arg)
{
	MyLogClass *Mylog = (MyLogClass *)arg;
	Mylog->log_thread_to_show();
	return NULL;
}

MyLogClass::MyLogClass(){
	log_fp = NULL;
	isNew = false;
	log_buffer_len = 0;
	_time = new utc_timer();
	log_data = new char[LOG_BUF_USE_LIMIT + LOG_LEN_LIMIT];
	log_buffer = new char[LOG_BUF_USE_LIMIT + LOG_LEN_LIMIT];

	InitMyLogClass();
	log_mutex = PTHREAD_MUTEX_INITIALIZER;
	log_cond = PTHREAD_COND_INITIALIZER;
	pthread_create(&log_thread, NULL, log_tread_func, this);
}

void MyLogClass::InitMyLogClass()
{
	std::string LogPath = "", LogName = "", CfgPath = "";
	memset(log_head_line, 0, sizeof(log_head_line));
	memset(log_head_end, 0, sizeof(log_head_end));
	if (GetModuleFileName(CfgPath, LogPath, LogName) == false){
		log_name	= LOG_NAME_HEAD;
		log_path	= LOG_PATH_DIR_NAME;
		CfgPath     	= "Config.ini";
	}else{
		log_name = LogName;
		log_path = LogPath + std::string(LOG_PATH_DIR_NAME);
	}
	IniFile::getInstance()->Load(CfgPath);

	log_write	= IniFile::getInstance()->GetIntValue("Log", "write", LOG_WRITE);
	log_debug	= IniFile::getInstance()->GetIntValue("Log", "debug", LOG_DEBUG);
	log_level	= IniFile::getInstance()->GetIntValue("Log", "level", LOG_LEVEL);
	log_days	= IniFile::getInstance()->GetIntValue("Log", "days", LOG_RETENTION_DAYS);

	if (log_write && log_debug) {
		log_head_line_len = snprintf(log_head_line, LOG_LEN_LIMIT, "%s[%s][%6lu] =================日志级别[%d], 开启写日志功能, 开启终端调试功能================", "[ INFO  ]", "xx:xx:xx.xxx", ThreadID, log_level);
	}else if (log_write && (!log_debug)) {
		log_head_line_len = snprintf(log_head_line, LOG_LEN_LIMIT, "%s[%s][%6lu] =================日志级别[%d], 开启写日志功能, 关闭终端调试功能================", "[ INFO  ]", "xx:xx:xx.xxx", ThreadID, log_level);
	}else if ((!log_write) && log_debug) {
		log_head_line_len = snprintf(log_head_line, LOG_LEN_LIMIT, "%s[%s][%6lu] =================日志级别[%d], 关闭写日志功能, 开启终端调试功能================", "[ INFO  ]", "xx:xx:xx.xxx", ThreadID, log_level);
	}else if ((!log_write) && (!log_debug)) {
		log_head_line_len = snprintf(log_head_line, LOG_LEN_LIMIT, "%s[%s][%6lu] =================日志级别[%d], 关闭写日志功能, 关闭终端调试功能================", "[ INFO  ]", "xx:xx:xx.xxx", ThreadID, log_level);
	}
	log_head_line[log_head_line_len++] = '\n';
	log_head_end_len = snprintf(log_head_end, LOG_LEN_LIMIT, "%s[%s][%6lu] =======日志目录 [%s] 剩余最大空间不超过 [%d] 关闭写日志功能=======", "[ WARN  ]", "xx:xx:xx.xxx", ThreadID, log_path.c_str(), LOG_MINIMUMSPACE);
	log_head_end[log_head_end_len++] = '\n';
}

void MyLogClass::log_thread_to_show()
{
	pthread_condattr_t attr;
	pthread_condattr_init(&attr);
	pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
	pthread_cond_init(&log_cond, &attr);

	memset(log_data, 0, LOG_BUF_USE_LIMIT + LOG_LEN_LIMIT);
	while(1){
		pthread_mutex_lock(&log_mutex);

		while (log_buffer_len == 0) {
			struct timespec tsp;
			memset(&tsp, 0, sizeof(tsp));
			clock_gettime(CLOCK_MONOTONIC, &tsp);
			tsp.tv_sec += LOG_BUFF_WAIT_TIME;
			pthread_cond_timedwait(&log_cond, &log_mutex, &tsp);
		}
		memcpy(log_data, log_buffer, log_buffer_len);
		log_data_len = log_buffer_len;
		log_buffer_len = 0;

		pthread_mutex_unlock(&log_mutex);

		if (log_debug) {
			std::cout << log_data << std::flush;
		}
		if (log_write) {
			write_to_file(_time->year, _time->mon, _time->day);
		}
		memset(log_data, 0, LOG_BUF_USE_LIMIT + LOG_LEN_LIMIT);
	}
}

bool MyLogClass::createdirectory(const std::string& dir_path)
{
	int dir_path_len = dir_path.length();
	if (dir_path_len > MAX_PATH_LEN){
		return false;
	}
	char tmp_path[MAX_PATH_LEN] = {0};
	for (int i = 0; i < dir_path_len; ++i){
		tmp_path[i] = dir_path[i];
		if (tmp_path[i] == '\\' || tmp_path[i] == '/'){
			if (ACCESS(tmp_path, 0) != 0){
				if (MKDIR(tmp_path) != 0){
					return false;
				}
			}
		}
	}
	return true;
}

bool MyLogClass::RemoveFile(char* filename, int max_days)
{
	int result;
	struct stat buf;

	result = stat(filename, &buf);
	if (result != 0) {
		switch (errno) {
			case ENOENT:
				log_message(ERROR, "file [%s] not found", filename);
				break;
			case EINVAL:
				log_message(ERROR, "file [%s] invalid parameter to stat", filename);
				break;
			default:
				log_message(ERROR, "file [%s] stat should never be reached", filename);
		}
		return false;
	}else {
		time_t t;
		tzset();
		t = time(NULL);
		time_t timeout_sec = (time_t)max_days * 60 * 60 * 24;
		if ((t - buf.st_mtime) > timeout_sec) {
			remove(filename);
			log_message(INFO, "Delete log file [%s] from 7 days ago", filename);
		}
		return true;
	}
}

bool MyLogClass::find_directory_to_del_old_files(const char *dir_name, const char* file_name, int max_days)
{
	DIR* dir;
	char buffer[PATH_MAX + 2];
	const char* src;
	char* p = buffer;
	bool file_exists = false;
	char* end = &buffer[PATH_MAX];

	src = dir_name;
	while (p < end && *src != '\0') {
		*p++ = *src++;
	}
	*p = '\0';

	dir = opendir(dir_name);
	if (dir != NULL) {
		struct dirent* ent;
		while ((ent = readdir(dir)) != NULL) {
			char* q = p;
			char c;
			if (buffer < q) {
				c = q[-1];
			}else {
				c = ':';
			}

			if (c != ':' && c != '/' && c != '\\') {
				*q++ = '/';
			}

			src = ent->d_name;
			while (q < end && *src != '\0') {
				*q++ = *src++;
			}
			*q = '\0';

			switch (ent->d_type) {
				case DT_LNK:
				case DT_REG:
					if ((strstr(buffer, file_name) != NULL)) {
						RemoveFile(buffer, max_days);
					}
					file_exists = true;
					break;
				case DT_DIR:
					if ((strcmp(ent->d_name, ".") != 0) && (strcmp(ent->d_name, "..") != 0)) {
						if (strlen(ent->d_name) == strlen("xxxx-xx-xx")){
							find_directory_to_del_old_files(buffer, file_name, max_days);
						}
						file_exists = true;
					}
					break;
			}
		}

		closedir(dir);
		if (file_exists == false){
			remove(dir_name);
			log_message(INFO, "remove empty directory [%s]", dir_name);
		}

		return true;
	}

	log_message(INFO, "Cannot open directory [%s]", dir_name);

	return false;
}

bool MyLogClass::write_to_file(int _year, int _mon, int _day)
{
	long long free_space;
	char name_buf[256] = {0};
	if (log_fp == NULL){
		count = 1;
		char secon_path[MAX_PATH_LEN] = {0};
		year = _year, mon = _mon, day = _day;
		snprintf(secon_path, 256, "%04d-%02d-%02d", year, mon, day);
		std::string log_name_path = log_path + std::string(SLASH) + std::string(secon_path) + std::string(SLASH);
		if (createdirectory(log_name_path) == true){
			std::string new_name = log_name + std::string();
			snprintf(name_buf, 256, "%s%s_%04d-%02d-%02d.log.%lu", log_name_path.c_str(), log_name.c_str(), year, mon, day, count);
			log_fp = fopen(name_buf, "a");
		}
		if(log_fp == NULL){
			std::cout << "fopen [" << name_buf << "] " << strerror(errno) << std::endl;
			return false;
		}
		isNew = true;
	}else if ((year + mon + day) != (_year + _mon + _day)){
		fclose(log_fp);
		count = 1;
		isNew = true;
		log_fp = NULL;
		char secon_path[MAX_PATH_LEN] = {0};
		year = _year, mon = _mon, day = _day;
		find_directory_to_del_old_files(log_path.c_str(), log_name.c_str(), log_days);
		snprintf(secon_path, 256, "%04d-%02d-%02d", year, mon, day);
		std::string log_name_path = log_path + std::string(SLASH) + std::string(secon_path) + std::string(SLASH);
		if (createdirectory(log_name_path) == true){
			std::string new_name = log_name + std::string();
			snprintf(name_buf, 256, "%s%s_%04d-%02d-%02d.log.%lu", log_name_path.c_str(), log_name.c_str(), year, mon, day, count);
			log_fp = fopen(name_buf, "a");
		}
		if(log_fp == NULL){
			std::cout << "fopen [" << name_buf << "] " << strerror(errno) << std::endl;
			return false;
		}
		isNew = true;
	}
	while(ftell(log_fp) >= LOG_USE_LIMIT){
		count++;
		fclose(log_fp);
		log_fp = NULL;
		char secon_path[MAX_PATH_LEN] = {0};
		year = _year, mon = _mon, day = _day;
		snprintf(secon_path, 256, "%04d-%02d-%02d", year, mon, day);
		std::string log_name_path = log_path + std::string(SLASH) + std::string(secon_path) + std::string(SLASH);
		if (createdirectory(log_name_path) == true){
			std::string new_name = log_name + std::string();
			char name_buf[256] = {0};
			snprintf(name_buf, 256, "%s%s_%04d-%02d-%02d.log.%lu", log_name_path.c_str(), log_name.c_str(), year, mon, day, count);
			log_fp = fopen(name_buf, "a");
		}
		if(log_fp == NULL){
			std::cout << "fopen [" << name_buf << "] " << strerror(errno) << std::endl;
			return false;
		}
		isNew = true;
	}

	if (isNew == true) {
		fwrite(log_head_line, 1, log_head_line_len, log_fp);
		isNew = false;
	}
	free_space = get_free_space_mb(log_path.c_str());
	if(free_space < LOG_MINIMUMSPACE){
		log_write = 0;
		fwrite(log_head_end, 1, log_head_end_len, log_fp);
		std::cout << "日志目录 [" << log_path << "] 剩余空间 [" << free_space << "MB] , 不满足最小值 [" << LOG_MINIMUMSPACE << "MB]要求，关闭写日志功能" << std::endl;
		return false;
	}

	if (fwrite(log_data, 1, log_data_len, log_fp) != log_data_len) {
		std::cout << "fwrite [" << name_buf << "] " << strerror(errno)  << std::endl;
		return false;
	}

	fflush(log_fp);

	return true;
}

void MyLogClass::try_append(char* log_line, size_t log_len)
{
	log_line[log_len++] = '\n';
	pthread_mutex_lock(&log_mutex);

	if (log_buffer_len < (LOG_BUF_USE_LIMIT - log_len)){
		memcpy(log_buffer + log_buffer_len, log_line, log_len);
		log_buffer_len += log_len;
		pthread_mutex_unlock(&log_mutex);
	}else {
		pthread_mutex_unlock(&log_mutex);
		pthread_cond_signal(&log_cond);
	}
}

void log_try_append(int lvl, unsigned long PID, const char* filename, int file_line, const char* func, const char* format, ...)
{
	int log_len;
	char log_line[LOG_LEN_LIMIT + 3] = {0};
	switch (lvl) {
		case TRACE:
			if (MyLogClass::getInstance()->log_level <= TRACE){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] ", "[ TRACE ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID);
			}else return;
			break;
		case INFO:
			if (MyLogClass::getInstance()->log_level <= INFO){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] ", "[ INFO  ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID);
			}else return;
			break;
		case WARN:
			if (MyLogClass::getInstance()->log_level <= WARN){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] ", "[ WARN  ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID);
			}else return;
			break;
		case ERROR:
			if (MyLogClass::getInstance()->log_level <= ERROR){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] %s:%d(%s) ", "[ ERROR ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID, filename, file_line, func);
			}else return;
			break;
		default:
			std::cout << "不存在此 [" << lvl << "] 日志等级" << std::endl;
			return;
	}
	va_list arg_ptr;
	va_start(arg_ptr, format);

	int main_len = vsnprintf(log_line + log_len, LOG_LEN_LIMIT - log_len, format, arg_ptr);
	va_end(arg_ptr);

	log_len += main_len;

	MyLogClass::getInstance()->try_append(log_line, log_len);
}

void hex_try_append(int lvl, unsigned long PID, const char* head, unsigned char* buffer, int buffer_len)
{
	int log_len;
	char log_line[LOG_LEN_LIMIT + 3] = {0};
	switch (lvl) {
		case TRACE:
			if (MyLogClass::getInstance()->log_level <= TRACE){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] %s ", "[ TRACE ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID, head);
			}else return;
			break;
		case INFO:
			if (MyLogClass::getInstance()->log_level <= INFO){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] %s ", "[ INFO  ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID, head);
			}else return;
			break;
		case WARN:
			if (MyLogClass::getInstance()->log_level <= WARN){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] %s ", "[ WARN  ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID, head);
			}else return;
			break;
		case ERROR:
			if (MyLogClass::getInstance()->log_level <= ERROR){
				log_len = snprintf(log_line, LOG_LEN_LIMIT, "%s[%s][%6lu] %s ", "[ ERROR ]", MyLogClass::getInstance()->_time->get_utc_timer().c_str(), PID, head);
			}else return;
			break;
		default:
			std::cout << "不存在此 [" << lvl << "] 日志等级" << std::endl;
			return;
	}
	for (int i = 0; (log_len < LOG_LEN_LIMIT - 4) && (i < buffer_len); i++){
		sprintf(log_line + log_len, " %02X", buffer[i]);
		log_len += 3;
	}
	MyLogClass::getInstance()->try_append(log_line, log_len);
}


