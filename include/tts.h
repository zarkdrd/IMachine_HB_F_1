/*************************************************************************
    > File Name: include/tts.h
    > Author: ARTC
    > Descripttion:
    > Created Time: 2024-04-30
 *************************************************************************/

#ifndef _INCLUDE_TTS_H_
#define _INCLUDE_TTS_H_

#include <iostream>
#include <thread>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>

#define TYPE_UTF8 1
#define TYPE_GBK  2

class TTS {
    public:
        TTS();
    	static TTS* getInstance();
        int compound(const char* text);
        void speech_playback(int nType, int nSpeed, void *text, int textLen);
        int text_to_speech(const char* src_text, const char* des_path, const char* params);
        void find_process_kill(const char *process_name);
    public:
        char m_text[2048];
        bool is_playing;
        std::mutex m_mutex;
        std::thread m_thread;
    private:
        static class TTS *m_Instance;
};

#endif //_INCLUDE_TTS_H_
