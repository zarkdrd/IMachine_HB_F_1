/*************************************************************************
    > File Name: src/tts.cpp
    > Author: ARTC
    > Descripttion:
    > Created Time: 2024-04-30
 ************************************************************************/

#include <thread>
#include <mutex>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "tts.h"
#include "tts/qtts.h"
#include "tts/msp_cmn.h"
#include "tts/msp_errors.h"
#include "convert.h"
#include "Log_Message.h"

typedef int SR_DWORD;
typedef short int SR_WORD ;

/* wav音频头部格式 */
typedef struct _wave_pcm_hdr
{
	char            riff[4];                // = "RIFF"
	int				size_8;                 // = FileSize - 8
	char            wave[4];                // = "WAVE"
	char            fmt[4];                 // = "fmt "
	int				fmt_size;				// = 下一个结构体的大小 : 16

	short int       format_tag;             // = PCM : 1
	short int       channels;               // = 通道数 : 1
	int				samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	int				avg_bytes_per_sec;      // = 每秒字节数 : samples_per_sec * bits_per_sample / 8
	short int       block_align;            // = 每采样点字节数 : wBitsPerSample / 8
	short int       bits_per_sample;        // = 量化比特数: 8 | 16

	char            data[4];                // = "data";
	int				data_size;              // = 纯数据长度 : FileSize - 44 
} wave_pcm_hdr;

/* 默认wav音频头部数据 */
wave_pcm_hdr default_wav_hdr =
{
	{ 'R', 'I', 'F', 'F' },
	0,
	{'W', 'A', 'V', 'E'},
	{'f', 'm', 't', ' '},
	16,
	1,
	1,
	16000,
	32000,
	2,
	16,
	{'d', 'a', 't', 'a'},
	0
};

TTS* TTS::m_Instance = NULL;

TTS::TTS()
{
    is_playing = false;
}

TTS* TTS::getInstance()
{
    if (m_Instance == NULL) {
        m_Instance = new TTS();
    }
    return m_Instance;
}

int TTS::text_to_speech(const char* src_text, const char* des_path, const char* params)
{
    int          ret          = -1;
	FILE*        fp           = NULL;
	const char*  sessionID    = NULL;
	unsigned int audio_len    = 0;
	wave_pcm_hdr wav_hdr      = default_wav_hdr;
	int          synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;

	if (NULL == src_text || NULL == des_path) {
        log_message(ERROR, "params is error");
		return ret;
	}
	fp = fopen(des_path, "wb");
	if (NULL == fp) {
        log_message(ERROR, "open %s error: %s", des_path, strerror(errno));
		return ret;
	}
	/* 开始合成 */
	sessionID = QTTSSessionBegin(params, &ret);
	if (MSP_SUCCESS != ret) {
        log_message(ERROR, "QTTSSessionBegin failed, error code: %d");
		fclose(fp);
		return ret;
	}
	ret = QTTSTextPut(sessionID, src_text, (unsigned int)strlen(src_text), NULL);
	if (MSP_SUCCESS != ret){
        log_message(ERROR, "QTTSTextPut failed, error code: %d", ret);
		QTTSSessionEnd(sessionID, "TextPutError");
		fclose(fp);
		return ret;
	}
	fwrite(&wav_hdr, sizeof(wav_hdr) ,1, fp);
	while (is_playing) {
		const void* data = QTTSAudioGet(sessionID, &audio_len, &synth_status, &ret);
		if (MSP_SUCCESS != ret) {
            break;
        }

		if (NULL != data) {
			fwrite(data, audio_len, 1, fp);
		    wav_hdr.data_size += audio_len;
		}
		if (MSP_TTS_FLAG_DATA_END == synth_status){
            break;
        }
	}
	if (MSP_SUCCESS != ret){
        log_message(ERROR, "QTTSAudioGet failed, error code: %d", ret);
		QTTSSessionEnd(sessionID, "AudioGetError");
		fclose(fp);
		return ret;
	}
	wav_hdr.size_8 += wav_hdr.data_size + (sizeof(wav_hdr) - 8);

	fseek(fp, 4, 0);
	fwrite(&wav_hdr.size_8,sizeof(wav_hdr.size_8), 1, fp);
	fseek(fp, 40, 0);
	fwrite(&wav_hdr.data_size,sizeof(wav_hdr.data_size), 1, fp);
    fflush(fp);
	fclose(fp);
	fp = NULL;
	ret = QTTSSessionEnd(sessionID, "Normal");
	if (MSP_SUCCESS != ret) {
        log_message(ERROR, "QTTSSessionEnd failed, error code: %d", ret);
	}

	return ret;
}

int TTS::compound(const char* text)
{
    int         ret                  = MSP_SUCCESS;
	const char* login_params         = "appid = f6760c2e, work_dir = .";
	const char* session_begin_params = "engine_type = local,voice_name=xiaoyan, text_encoding = UTF8, tts_res_path =fo|res/tts/xiaoyan.jet;fo|res/tts/common.jet , sample_rate = 16000, speed = 50, volume = 50, pitch = 50, rdn = 2";
	const char* filename             = "tts_sample.wav";

	ret = MSPLogin(NULL, NULL, login_params);
    if (MSP_SUCCESS != ret) {
        log_message(ERROR, "MSPLogin failed, error code: %d.", ret);
		goto exit ;
	}

	log_message(INFO, "开始合成语音...");
	ret = text_to_speech(text, filename, session_begin_params);
	if (MSP_SUCCESS != ret) {
        log_message(ERROR, "语音合成失败");
        goto exit;
	}
    log_message(INFO, "语音合成完毕.");
    system("aplay tts_sample.wav >/dev/null 2>&1 &");
exit:
	MSPLogout();
	return 0;
}

void TTS::find_process_kill(const char *process_name)
{
    char cmd[256] = {0};
    snprintf(cmd, sizeof(cmd), "pidof %s", process_name);
    int status = system(cmd);
    if (status == 0) {
        system("killall -9 aplay");
    }
}
void tts_thread(TTS *tts)
{
    tts->is_playing = true;
    tts->compound(tts->m_text);
}
void TTS::speech_playback(int nType, int nSpeed, void *text, int textLen){
	char speech_text[textLen * 3 + 3] = {0};
	if (nType == TYPE_UTF8) {
		memcpy(speech_text, text, textLen);
	}else if(nType == TYPE_GBK){
		char speech_data[textLen + 1] = {0};
		memcpy(speech_data, text, textLen);
		charset_convert_GB2312_TO_UTF8(speech_data, textLen, speech_text, sizeof(speech_text));
	}
    m_mutex.lock();
    if (m_thread.joinable()){
        m_thread.join();
        find_process_kill("aplay");
    }
	char SoundLevel[256] = {0};
	sprintf(SoundLevel, "amixer set Master %d0%", nSpeed);
	system(SoundLevel);
	log_message(INFO, "声音级别[%d]语音播放: %s", nSpeed, speech_text);
    memset(this->m_text, 0, sizeof(this->m_text));
    snprintf(this->m_text, sizeof(this->m_text), "%s", speech_text);
    m_thread = std::thread(tts_thread, this);
    m_mutex.unlock();
}