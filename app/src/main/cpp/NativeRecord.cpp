//
// Created by Administrator on 2017/8/4.
//
#include <fcntl.h>

#define LOG_TAG "NativeRecord"

#include <cstdio>
#include <cstdlib>
#include "NativeRecord.h"
#include "Log.h"

NativeRecord::NativeRecord(Arguments *arguments)
        : arguments(arguments) {
    fpsTime = 1000 / FRAME_RATE;
    pthread_mutex_init(&media_mutex,NULL);
}

NativeRecord::~NativeRecord() {

}

int64_t systemnanotime() {
    timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec * 1000000000LL + now.tv_nsec;
}

bool NativeRecord::prepare() {
    AMediaFormat *audioFormat = AMediaFormat_new();
    AMediaFormat_setString(audioFormat, AMEDIAFORMAT_KEY_MIME, AUDIO_MIME);
    AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, SAMPLE_RATE);
    AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_AAC_PROFILE, AAC_PROFILE);
    AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_BIT_RATE, AUDIO_BIT_RATE);
    AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
//    AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_IS_ADTS, 0);
//    uint8_t es[2] = {0x12, 0x12};
//    AMediaFormat_setBuffer(audioFormat, "csd-0", es, 2);
    audioCodec = AMediaCodec_createEncoderByType(AUDIO_MIME);
    AMediaCodec_configure(audioCodec, audioFormat, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    LOGI("音频初始化完毕");


    AMediaFormat *videoFormat = AMediaFormat_new();
    AMediaFormat_setString(videoFormat, AMEDIAFORMAT_KEY_MIME, VIDEO_MIME);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_WIDTH, arguments->width);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_HEIGHT, arguments->height);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_BIT_RATE,
                          (int) (BPP * FRAME_RATE * arguments->width * arguments->height));
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_FRAME_RATE, FRAME_RATE);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, I_FRAME_INTERVAL);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, COLOR_FORMAT);
//    AMediaFormat_setBuffer(videoFormat, "csd-0", sps, spsSize); // sps
//    AMediaFormat_setBuffer(videoFormat, "csd-1", pps, ppsSize); // pps
    videoCodec = AMediaCodec_createEncoderByType(VIDEO_MIME);
    AMediaCodec_configure(videoCodec, videoFormat, NULL, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    LOGI("视频初始化完毕");


    int fd = open(arguments->path, O_CREAT | O_RDWR, 0666);
    if (!fd) {
        LOGI("打开文件出错");
        return false;
    }
    mMuxer = AMediaMuxer_new(fd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
    AMediaMuxer_setOrientationHint(mMuxer, 180);
    return true;
}

void NativeRecord::feedData(void *data) {
    nowFeedData = data;
}

int NativeRecord::start() {
    nanoTime = systemnanotime();

    pthread_mutex_lock(&media_mutex);
    media_status_t audioStatus = AMediaCodec_start(audioCodec);
    LOGI("开启音频编码%d", audioStatus);
    media_status_t videoStatus = AMediaCodec_start(videoCodec);
    LOGI("开启视频编码%d", videoStatus);

    arguments->jniEnv->CallVoidMethod(arguments->mAudioRecord, arguments->startRecording_id);

    isRecording = true;
    mStartFlag = true;

    if (mAudioThread){
        pthread_join(mAudioThread, NULL);
        mAudioThread = NULL;
    }
    if (mVideoThread){
        pthread_join(mVideoThread, NULL);
        mVideoThread = NULL;
    }
    pthread_create(&mAudioThread, NULL, NativeRecord::audioStep, this);
    pthread_create(&mVideoThread, NULL, NativeRecord::videoStep, this);
    pthread_mutex_unlock(&media_mutex);
    return 0;
}

void NativeRecord::stop() {
    LOGI("stop");
    pthread_mutex_lock(&media_mutex);
    isRecording = false;
    mStartFlag = false;
    if (pthread_join(mAudioThread, NULL) != EXIT_SUCCESS) {
        LOGE("NativeRecord::terminate audio thread: pthread_join failed");
    }
    if (pthread_join(mVideoThread, NULL) != EXIT_SUCCESS) {
        LOGE("NativeRecord::terminate video thread: pthread_join failed");
    }
    if (arguments->mAudioRecord != NULL) {
        arguments->jniEnv->CallVoidMethod(arguments->mAudioRecord, arguments->stop_id);
        arguments->jniEnv->CallVoidMethod(arguments->mAudioRecord, arguments->release_id);
        arguments->jniEnv->DeleteGlobalRef(arguments->mAudioRecord);
        arguments->mAudioRecord = NULL;
    }

    if (audioCodec != NULL) {
        AMediaCodec_stop(audioCodec);
        AMediaCodec_delete(audioCodec);
        audioCodec = NULL;
    }

    if (videoCodec != NULL) {
        AMediaCodec_stop(videoCodec);
        AMediaCodec_delete(videoCodec);
        videoCodec = NULL;
    }

    mVideoTrack = -1;
    mAudioTrack = -1;

    if (mMuxer != NULL) {
        AMediaMuxer_stop(mMuxer);
        AMediaMuxer_delete(mMuxer);
        mMuxer = NULL;
    }
    pthread_mutex_unlock(&media_mutex);
    pthread_mutex_destroy(&media_mutex);
    LOGI("finish");
}

void *NativeRecord::audioStep(void *obj) {
    LOGI("audioStep");
    NativeRecord *record = (NativeRecord *) obj;
    while (record->mStartFlag) {
        ssize_t index = AMediaCodec_dequeueInputBuffer(record->audioCodec, -1);
        size_t out_size;
        if (index >= 0) {
            uint8_t *inputBuffer = AMediaCodec_getInputBuffer(record->audioCodec, index, &out_size);
            //开始读取数据
            JNIEnv *env;
            record->arguments->javaVM->AttachCurrentThread(&env, NULL);
            jbyteArray temp = env->NewByteArray(out_size);
            jint length = env->CallIntMethod(record->arguments->mAudioRecord,
                                             record->arguments->read_id, temp, 0,
                                             out_size);
            jbyte *audio_bytes = env->GetByteArrayElements(temp, NULL);
            if (length > 0) {
                memcpy(inputBuffer, audio_bytes, out_size);
                env->ReleaseByteArrayElements(temp, audio_bytes, 0);
                AMediaCodec_queueInputBuffer(record->audioCodec, index, 0, out_size,
                                             (systemnanotime() - record->nanoTime) / 1000,
                                             record->isRecording ? 0
                                                                 : AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
            }
            record->arguments->javaVM->DetachCurrentThread();
        }

        AMediaCodecBufferInfo *info = (AMediaCodecBufferInfo *) malloc(
                sizeof(AMediaCodecBufferInfo));
        ssize_t outIndex;
        do {
            outIndex = AMediaCodec_dequeueOutputBuffer(record->audioCodec, info, 0);
            size_t out_size;
            if (outIndex >= 0) {
                uint8_t *outputBuffer = AMediaCodec_getOutputBuffer(record->audioCodec, outIndex,
                                                                    &out_size);
                if (record->mAudioTrack >= 0 && record->mVideoTrack >= 0 && info->size > 0 &&
                    info->presentationTimeUs > 0) {
                    media_status_t status = AMediaMuxer_writeSampleData(record->mMuxer, record->mAudioTrack, outputBuffer,
                                                                        info);
                    LOGE("音频状态====%d",status);
                }
                AMediaCodec_releaseOutputBuffer(record->audioCodec, outIndex, false);
                if (record->isRecording) {
                    continue;
                }
            } else if (outIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                record->mAudioTrack = AMediaMuxer_addTrack(record->mMuxer,
                                                           AMediaCodec_getOutputFormat(
                                                                   record->audioCodec));
                LOGE("add audio track-->%d", record->mAudioTrack);
                if (record->mAudioTrack >= 0 && record->mVideoTrack >= 0) {
                    AMediaMuxer_start(record->mMuxer);
                }
            }
        } while (outIndex >= 0);
    }
    return 0;
}


void *NativeRecord::videoStep(void *obj) {
    LOGI("videoStep");
    NativeRecord *record = (NativeRecord *) obj;
    while (record->mStartFlag) {
        int64_t time = systemnanotime();
        ssize_t index = AMediaCodec_dequeueInputBuffer(record->videoCodec, -1);
        size_t out_size;
        if (index >= 0) {
            uint8_t *buffer = AMediaCodec_getInputBuffer(record->videoCodec, index, &out_size);
            if (record->nowFeedData != NULL) {
                memcpy(buffer, record->nowFeedData, out_size);
            }
            AMediaCodec_queueInputBuffer(record->videoCodec, index, 0, out_size,
                                         (systemnanotime() - record->nanoTime) / 1000,
                                         record->isRecording ? 0
                                                             : AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        }
        AMediaCodecBufferInfo *info = (AMediaCodecBufferInfo *) malloc(
                sizeof(AMediaCodecBufferInfo));
        ssize_t outIndex;
        do {
            outIndex = AMediaCodec_dequeueOutputBuffer(record->videoCodec, info, 0);
            size_t out_size;
            if (outIndex >= 0) {
                uint8_t *outputBuffer = AMediaCodec_getOutputBuffer(record->videoCodec, outIndex,
                                                                    &out_size);
                if (record->mAudioTrack >= 0 && record->mVideoTrack >= 0 && info->size > 0 &&
                    info->presentationTimeUs > 0) {
                    media_status_t status = AMediaMuxer_writeSampleData(record->mMuxer, record->mVideoTrack, outputBuffer,
                                                                        info);
                    LOGE("视频状态====%d",status);
                }
                AMediaCodec_releaseOutputBuffer(record->videoCodec, outIndex, false);
                if (record->isRecording) {
                    continue;
                }
            } else if (outIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                record->mVideoTrack = AMediaMuxer_addTrack(record->mMuxer,
                                                           AMediaCodec_getOutputFormat(
                                                                   record->videoCodec));
                LOGE("add video track-->%d", record->mVideoTrack);
                if (record->mAudioTrack >= 0 && record->mVideoTrack >= 0) {
                    AMediaMuxer_start(record->mMuxer);
                }
            }
        } while (outIndex >= 0);
        int64_t lt = systemnanotime() - time;
        if (record->fpsTime > lt) {
            usleep(record->sleepTime);
        }
    }
    return 0;
}