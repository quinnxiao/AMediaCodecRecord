
/**
 * Created by jianxi on 2017/6/2.
 * https://github.com/mabeijianxi
 * mabeijianxi@gmail.com
 */
#ifndef JIANXIFFMPEG_JX_LOG_H
#define JIANXIFFMPEG_JX_LOG_H
#include <jni.h>
#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <libgen.h>
#include <unistd.h>

static int JNI_DEBUG = 1;
#define LOG_TAG "NativeRecord"

#define LOGV(FMT, ...) if(JNI_DEBUG) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGD(FMT, ...) if(JNI_DEBUG) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGI(FMT, ...) if(JNI_DEBUG) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGW(FMT, ...) if(JNI_DEBUG) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGE(FMT, ...) if(JNI_DEBUG) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)
#define LOGF(FMT, ...) if(JNI_DEBUG) __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, "[%d*%s:%d:%s]:" FMT,	\
							gettid(), basename(__FILE__), __LINE__, __FUNCTION__, ## __VA_ARGS__)

#endif //JIANXIFFMPEG_JX_LOG_H
