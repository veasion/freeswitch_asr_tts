/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005-2014, Veasion <veasion@qq.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Veasion <veasion@qq.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Veasion <veasion@qq.com>
 *
 * mod_ali_tts.cpp -- Aliyun Interface
 *
 */

#include <switch.h>
#include <bitset>
#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <fstream>
#include <nlsClient.h>
#include <nlsEvent.h>
#include <speechSynthesizerRequest.h>
#include <nlsToken.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


using namespace std;
using namespace AlibabaNlsCommon;

using AlibabaNls::NlsClient;
using AlibabaNls::NlsEvent;
using AlibabaNls::LogDebug;
using AlibabaNls::LogInfo;
using AlibabaNls::SpeechSynthesizerCallback;
using AlibabaNls::SpeechSynthesizerRequest;
using AlibabaNlsCommon::NlsToken;

/* module name */
#define MOD_NAME "ali_tts"
/* module config file */
#define CONFIG_FILE "ali_tts.conf"

static struct {
    char* app_key;
    char* access_key;
    char* key_secret;
    int thread_count;
    int cache_size;
} globals;

SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_app_key, globals.app_key);
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_access_key, globals.access_key);
SWITCH_DECLARE_GLOBAL_STRING_FUNC(set_global_key_secret, globals.key_secret);

SWITCH_MODULE_LOAD_FUNCTION(mod_ali_tts_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ali_tts_shutdown);
SWITCH_MODULE_DEFINITION(mod_ali_tts, mod_ali_tts_load, mod_ali_tts_shutdown, NULL);

struct ali_tts_config {
    char* app_key;
    char* access_key;
    char* key_secret;
    char* voice;
    char* format;
    int sample_rate;
    int volume;
    int speech_rate;
    int pitch_rate;
    char* voice_file;
    int voice_cursor;
};

// 自定义事件回调参数
struct ParamCallBack {
    std::string binAudioFile;
    std::ofstream audioFile;
	SpeechSynthesizerRequest* request;
};

typedef struct ali_tts_config ali_config;

static switch_status_t ali_do_config(void) {
    switch_xml_t cfg, xml, settings, param;
    globals.thread_count = 4;
    globals.cache_size = 1600;
    globals.app_key = NULL;
    globals.access_key = NULL;
    globals.key_secret = NULL;
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "load ali_tts config");
    
    if ((xml = switch_xml_open_cfg(CONFIG_FILE, &cfg, NULL))) {
		if ((settings = switch_xml_child(cfg, "settings"))) {
            for (param = switch_xml_child(settings, "param"); param; param = param->next) {
                char *var = (char *) switch_xml_attr_soft(param, "name");
                char *val = (char *) switch_xml_attr_soft(param, "value");
                if (!strcmp(var, "thread_count")) {
                    globals.thread_count = atoi(val);
                } else if (!strcmp(var, "cache_size")) {
                    globals.cache_size = atoi(val);
                } else if (!strcmp(var, "app_key")) {
                    set_global_app_key(val);
                } else if (!strcmp(var, "access_key")) {
                    set_global_access_key(val);
                } else if (!strcmp(var, "key_secret")) {
                    set_global_key_secret(val);
                }
            }
        }
        switch_xml_free(xml);
    } else {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Open of %s failed\n", CONFIG_FILE);
    }
    
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t ali_speech_open(switch_speech_handle_t *sh, const char *voice_name, int rate, int channels, switch_speech_flag_t *flags)
{
    ali_config *conf = (ali_config *) switch_core_alloc(sh->memory_pool, sizeof(ali_config));
    conf->voice = switch_core_strdup(sh->memory_pool, voice_name);
    conf->sample_rate = rate;
    conf->volume = 80;
    conf->speech_rate = 0;
    conf->pitch_rate = 0;
    conf->format = switch_core_strdup(sh->memory_pool, "wav");
    conf->app_key = switch_core_strdup(sh->memory_pool, globals.app_key);
    conf->access_key = switch_core_strdup(sh->memory_pool, globals.access_key);
    conf->key_secret = switch_core_strdup(sh->memory_pool, globals.key_secret);
    sh->private_info = conf;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_speech_open.\n");
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t ali_speech_close(switch_speech_handle_t *sh, switch_speech_flag_t *flags)
{
	// delete file
	ali_config *ali = (ali_config *) sh->private_info;
    // switch_file_remove(ali->voice_file, NULL);
	unlink(ali->voice_file);
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_speech_close.\n");
    return SWITCH_STATUS_SUCCESS;
}

static void on_completed(NlsEvent* cbEvent, void* cbParam) {
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_tts on_completed.\n");
}

static void on_closed(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    tmpParam->audioFile.close();
	// 这个释放 request 有 bug，会卡住
    // NlsClient::getInstance()->releaseSynthesizerRequest(tmpParam->request);
    delete tmpParam;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_tts on_closed.\n");
}

static void on_failed(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    delete tmpParam;
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_tts on_failed.\n");
}

static void on_received(NlsEvent* cbEvent, void* cbParam) {
    ParamCallBack* tmpParam = (ParamCallBack*)cbParam;
    const vector<unsigned char>& data = cbEvent->getBinaryData();
    if (data.size() > 0) {
        tmpParam->audioFile.write((char*)&data[0], data.size());
        tmpParam->audioFile.flush();
    }
	// switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_tts on_received: status code=%d, task id=%s, data size=%ld.\n", cbEvent->getStatusCode(), cbEvent->getTaskId(), data.size());
}

static string ali_get_token(const char* accessKey, const char* keySecret) {
    string token;
    NlsToken nlsTokenRequest;
    nlsTokenRequest.setAccessKeyId(accessKey);
    nlsTokenRequest.setKeySecret(keySecret);
    if (-1 != nlsTokenRequest.applyNlsToken()) {
        token = nlsTokenRequest.getToken();
    }
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali token: %s\n", token.c_str());
    return token;
}

static int ali_file_size(const char* fileName) {
    int size = 0; 
    FILE* file = fopen(fileName, "rb");
    if (file) {
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fclose(file);
    }
    return size;
}

static size_t ali_file_read(const char* fileName, int start, void* data, size_t len) {
    size_t read_size = 0; 
    FILE* file = fopen(fileName, "rb");
    if (file) {
        fseek(file, start, SEEK_SET);
        read_size = fread(data, 1, len, file);
        fclose(file);
    }
    return read_size;
}

static switch_status_t ali_cloud_tts(const char* appKey, const char* accessKey, const char* keySecret, const char* voice, int volume, const char* format, int speechRate, int pitchRate, int sampleRate, const char* text, const string& file) {
    SpeechSynthesizerRequest* request = NlsClient::getInstance()->createSynthesizerRequest();
    if (request) {
        ParamCallBack* cbParam = new ParamCallBack;
		cbParam->request = request;
        cbParam->binAudioFile = file;
        cbParam->audioFile.open(cbParam->binAudioFile.c_str(), std::ios::binary | std::ios::out);
		// 设置URL
		request->setUrl("wss://nls-gateway-cn-shanghai.aliyuncs.com/ws/v1");
        // 设置音频合成结束回调函数
        request->setOnSynthesisCompleted(on_completed, cbParam);
        // 设置音频合成通道关闭回调函数
        request->setOnChannelClosed(on_closed, cbParam);
        // 设置异常失败回调函数
        request->setOnTaskFailed(on_failed, cbParam);
        // 设置文本音频数据接收回调函数
        request->setOnBinaryDataReceived(on_received, cbParam);
        // 设置AppKey, 必填参数
        request->setAppKey(appKey);
        // 设置账号校验token, 必填参数
        request->setToken(ali_get_token(accessKey, keySecret).c_str());
        // 发音人, 包含"xiaoyun", "ruoxi", "xiaogang", "aixia"等. 可选参数, 默认是xiaoyun
        request->setVoice(voice);
        // 音量, 范围是0~100, 可选参数, 默认50
        request->setVolume(volume);
        // 设置音频数据编码格式, 可选参数, 目前支持pcm, opus. 默认是pcm
        request->setFormat(format);
        // 语速, 范围是-500~500, 可选参数, 默认是0
        request->setSpeechRate(speechRate);
        // 语调, 范围是-500~500, 可选参数, 默认是0
        request->setPitchRate(pitchRate);
        // 设置音频数据采样率, 可选参数, 目前支持16000, 8000. 默认是16000
        request->setSampleRate(sampleRate);
        // 转换文本
        request->setText(text);
        // tts start
		int status = request->start();
        if (status >= 0) {
			// success
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_tts request success: %d\n", status);
            return SWITCH_STATUS_SUCCESS;
        } else {
			// error
			request->stop();
			NlsClient::getInstance()->releaseSynthesizerRequest(request);
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ali_tts request error: %d\n", status);
		}
    }
    
    return SWITCH_STATUS_FALSE;
}

static string ali_md5(const char* data) {
    char digest[SWITCH_MD5_DIGEST_STRING_SIZE] = { 0 };
    switch_md5_string(digest, (void *) data, strlen(data));
    return string(digest);
}

// start tts
static switch_status_t ali_speech_feed_tts(switch_speech_handle_t *sh, char *text, switch_speech_flag_t *flags)
{
    ali_config *ali = (ali_config *) sh->private_info;
	
    string voice_file = "/tmp/ali_tts/";
	
	if (access(voice_file.c_str(), 0) == -1) {
		mkdir(voice_file.c_str(), S_IRWXU | S_IRWXG | S_IROTH);
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "mkdir tts_audio_path: %s\n", voice_file.c_str());
	}
	
    voice_file += ali_md5(text);
    voice_file += ".wav";
	
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_tts_speech_feed_tts audio_file: %s\n", voice_file.c_str());

    ali->voice_file = switch_core_strdup(sh->memory_pool, voice_file.c_str());
    ali->voice_cursor = 0;

    return ali_cloud_tts(ali->app_key, ali->access_key, ali->key_secret, ali->voice, ali->volume, ali->format, ali->speech_rate, ali->pitch_rate, ali->sample_rate, text, voice_file);
}

// stop tts
static void ali_speech_flush_tts(switch_speech_handle_t *sh)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "ali_speech_flush_tts.\n");
}

// read tts data
static switch_status_t ali_speech_read_tts(switch_speech_handle_t *sh, void *data, size_t *datalen, switch_speech_flag_t *flags)
{
    ali_config *ali = (ali_config *) sh->private_info;
    assert(ali != NULL);
    if (ali_file_size(ali->voice_file) >= globals.cache_size) {
        size_t read_size = ali_file_read(ali->voice_file, ali->voice_cursor, data, *datalen);
        if (read_size == 0) {
            return SWITCH_STATUS_BREAK;
        }
        ali->voice_cursor += read_size;
    }
    return SWITCH_STATUS_SUCCESS;
}

static void ali_text_param_tts(switch_speech_handle_t *sh, char *param, const char *val)
{
    ali_config *ali = (ali_config *) sh->private_info;

    if (!strcasecmp(param, "app_key")) {
        ali->app_key = switch_core_strdup(sh->memory_pool, val);
    } else if (!strcasecmp(param, "access_key")) {
        ali->access_key = switch_core_strdup(sh->memory_pool, val);
    } else if (!strcasecmp(param, "key_secret")) {
        ali->key_secret = switch_core_strdup(sh->memory_pool, val);
    } else if (!strcasecmp(param, "voice")) {
        ali->voice = switch_core_strdup(sh->memory_pool, val);
    } else if (!strcasecmp(param, "format")) {
        ali->format = switch_core_strdup(sh->memory_pool, val);
    } else if (!strcasecmp(param, "sample_rate")) {
        ali->sample_rate = atoi(val);
    } else if (!strcasecmp(param, "volume")) {
        ali->volume = atoi(val);
    } else if (!strcasecmp(param, "speech_rate")) {
        ali->speech_rate = atoi(val);
    } else if (!strcasecmp(param, "pitch_rate")) {
        ali->pitch_rate = atoi(val);
    }
    // set params
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_ali_tts ----- text param=%s, val=%s\n", param, val);
}

static void ali_numeric_param_tts(switch_speech_handle_t *sh, char *param, int val)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_ali_tts ----- numeric param=%s, val=%d\n", param, val);
}

static void ali_float_param_tts(switch_speech_handle_t *sh, char *param, double val)
{
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_ali_tts ----- float param=%s, val=%f\n", param, val);
}

SWITCH_MODULE_LOAD_FUNCTION(mod_ali_tts_load)
{
	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "load ali_tts start...");
	
    switch_speech_interface_t *speech_interface;

    *module_interface = switch_loadable_module_create_module_interface(pool, modname);
    speech_interface = (switch_speech_interface_t *)switch_loadable_module_create_interface(*module_interface, SWITCH_SPEECH_INTERFACE);
    speech_interface->interface_name = MOD_NAME;
    speech_interface->speech_open = ali_speech_open;
    speech_interface->speech_close = ali_speech_close;
    speech_interface->speech_feed_tts = ali_speech_feed_tts;
    speech_interface->speech_read_tts = ali_speech_read_tts;
    speech_interface->speech_flush_tts = ali_speech_flush_tts;
    speech_interface->speech_text_param_tts = ali_text_param_tts;
    speech_interface->speech_numeric_param_tts = ali_numeric_param_tts;
    speech_interface->speech_float_param_tts = ali_float_param_tts;

    // read config
    ali_do_config();
    // init
    NlsClient::getInstance()->setLogConfig("ali-speech", LogDebug);
    NlsClient::getInstance()->startWorkThread(globals.thread_count);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "load ali_tts end.");
	
    return SWITCH_STATUS_SUCCESS;
}

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_ali_tts_shutdown)
{
    NlsClient::releaseInstance();
    switch_safe_free(globals.app_key);
    switch_safe_free(globals.access_key);
    switch_safe_free(globals.key_secret);
    return SWITCH_STATUS_UNLOAD;
}
