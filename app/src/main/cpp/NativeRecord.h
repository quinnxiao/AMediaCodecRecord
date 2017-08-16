//
// Created by Administrator on 2017/8/4.
//

#ifndef INC_720VV_O2_NATIVERECORD_H
#define INC_720VV_O2_NATIVERECORD_H

#define SAMPLE_RATE (48000)//音频采样率
#define AUDIO_BIT_RATE (128000)//音频编码的比特率
#define CHANNEL_COUNT (2)//音频编码通道数
#define CHANNEL_CONFIG (12)//音频录制通道,默认为立体声
#define AAC_PROFILE (2)
#define AUDIO_FORMAT (2)//音频录制格式，默认为PCM16Bit
#define AUDIO_SOURCE (1)


#define FRAME_RATE (30)
#define I_FRAME_INTERVAL (1)
#define COLOR_FORMAT (21)

#include <media/NdkMediaMuxer.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <pthread.h>
#include <jni.h>
#include "Arguments.h"


class NativeRecord {
private:
    AMediaMuxer *mMuxer;  //多路复用器，用于音视频混合
    AMediaCodec *audioCodec;   //编码器，用于音频编码
    AMediaCodec *videoCodec;   //编码器，用于视频编码
    Arguments *arguments;

    const char *AUDIO_MIME = "audio/mp4a-latm", *VIDEO_MIME = "video/avc";


    int mAudioTrack = -1, mVideoTrack = -1;

    pthread_t mAudioThread, mVideoThread;
    pthread_mutex_t media_mutex;

    void *nowFeedData;

    const float BPP = 0.25f;

    int64_t fpsTime;
    uint sleepTime = 20*1000;

    bool isRecording = false,mStartFlag = false;

    int64_t nanoTime;

    int64_t duration;

public:
    NativeRecord(Arguments *arguments);

    ~NativeRecord();

    bool prepare();

    int start();

    void stop();

    void feedData(void *data);


    static void* audioStep(void *obj);

    static void* videoStep(void *obj);

};


#endif //INC_720VV_O2_NATIVERECORD_H
