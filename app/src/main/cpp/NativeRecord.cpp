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
    fpsTime = 1000 / arguments->frameRate;
    pthread_mutex_init(&media_mutex, NULL);
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
    AMediaFormat_setInt32(audioFormat, AMEDIAFORMAT_KEY_IS_ADTS, 0);
    uint8_t es[2] = {0x12, 0x12};
    AMediaFormat_setBuffer(audioFormat, "csd-0", es, 2);
    media_status_t audioConfigureStatus = AMediaCodec_configure(audioCodec, audioFormat, NULL, NULL,
                                                                AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    if (AMEDIA_OK != audioConfigureStatus) {
        LOGE("set audio configure failed status-->%d", audioConfigureStatus);
        return false;
    }
    AMediaFormat_delete(audioFormat);
    LOGI("init audioCodec success");


    AMediaFormat *videoFormat = AMediaFormat_new();
    AMediaFormat_setString(videoFormat, AMEDIAFORMAT_KEY_MIME, VIDEO_MIME);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_WIDTH, arguments->width);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_HEIGHT, arguments->height);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_BIT_RATE, arguments->bitRate);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_FRAME_RATE, arguments->frameRate);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, I_FRAME_INTERVAL);
    AMediaFormat_setInt32(videoFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, arguments->colorFormat);
    uint8_t sps[2] = {0x12, 0x12};
    uint8_t pps[2] = {0x12, 0x12};
    AMediaFormat_setBuffer(videoFormat, "csd-0", sps, 2); // sps
    AMediaFormat_setBuffer(videoFormat, "csd-1", pps, 2); // pps
    videoCodec = AMediaCodec_createEncoderByType(VIDEO_MIME);
    media_status_t videoConfigureStatus = AMediaCodec_configure(videoCodec, videoFormat, NULL, NULL,
                                                                AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    if (AMEDIA_OK != videoConfigureStatus) {
        LOGE("set video configure failed status-->%d", videoConfigureStatus);
        return false;
    }
    AMediaFormat_delete(videoFormat);
    LOGI("init videoCodec success");


    int fd = open(arguments->path, O_CREAT | O_RDWR, 0666);
    if (!fd) {
        LOGE("open media file failed-->%d", fd);
        return false;
    }
    mMuxer = AMediaMuxer_new(fd, AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4);
    AMediaMuxer_setOrientationHint(mMuxer, 180);
    return true;
}

void NativeRecord::feedData(void *data) {
    frame_queue.push(data);
}

bool NativeRecord::start() {
    nanoTime = systemnanotime();

    pthread_mutex_lock(&media_mutex);
    media_status_t audioStatus = AMediaCodec_start(audioCodec);
    if (AMEDIA_OK != audioStatus) {
        LOGI("open audioCodec status-->%d", audioStatus);
        return false;
    }
    media_status_t videoStatus = AMediaCodec_start(videoCodec);
    if (AMEDIA_OK != videoStatus) {
        LOGI("open videoCodec status-->%d", videoStatus);
        return false;
    }

    arguments->jniEnv->CallVoidMethod(arguments->mAudioRecord, arguments->startRecording_id);

    mIsRecording = true;
    mStartFlag = true;

    if (mAudioThread) {
        pthread_join(mAudioThread, NULL);
        mAudioThread = NULL;
    }
    if (mVideoThread) {
        pthread_join(mVideoThread, NULL);
        mVideoThread = NULL;
    }
    pthread_create(&mVideoThread, NULL, NativeRecord::videoStep, this);
    pthread_create(&mAudioThread, NULL, NativeRecord::audioStep, this);
    pthread_mutex_unlock(&media_mutex);
    return true;
}

bool NativeRecord::isRecording() {
    return mIsRecording;
}

void NativeRecord::stop() {
    LOGI("stop");
    pthread_mutex_lock(&media_mutex);
    mIsRecording = false;
    mStartFlag = false;

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

    if (pthread_join(mAudioThread, NULL) != EXIT_SUCCESS) {
        LOGE("NativeRecord::terminate audio thread: pthread_join failed");
    }
    if (pthread_join(mVideoThread, NULL) != EXIT_SUCCESS) {
        LOGE("NativeRecord::terminate video thread: pthread_join failed");
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
                                             record->mIsRecording ? 0
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
                    AMediaMuxer_writeSampleData(record->mMuxer, record->mAudioTrack,
                                                outputBuffer,
                                                info);
                }
                AMediaCodec_releaseOutputBuffer(record->audioCodec, outIndex, false);
                if (record->mIsRecording) {
                    continue;
                }
            } else if (outIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                AMediaFormat *outFormat = AMediaCodec_getOutputFormat(record->audioCodec);
                ssize_t track = AMediaMuxer_addTrack(record->mMuxer, outFormat);
                const char *s = AMediaFormat_toString(outFormat);
                record->mAudioTrack = 1;
                LOGI("audio out format %s", s);
                LOGE("add audio track status-->%d", track);
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
        if (record->frame_queue.empty()) continue;
//        int64_t time = systemnanotime();
        ssize_t index = AMediaCodec_dequeueInputBuffer(record->videoCodec, -1);
        size_t out_size;
        if (index >= 0) {
            uint8_t *buffer = AMediaCodec_getInputBuffer(record->videoCodec, index, &out_size);
            void *data = *record->frame_queue.wait_and_pop().get();
            if (data != NULL) {
                memcpy(buffer, data, out_size);
            }
            AMediaCodec_queueInputBuffer(record->videoCodec, index, 0, out_size,
                                         (systemnanotime() - record->nanoTime) / 1000,
                                         record->mIsRecording ? 0
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
                    AMediaMuxer_writeSampleData(record->mMuxer, record->mVideoTrack,
                                                outputBuffer,
                                                info);
                }
                AMediaCodec_releaseOutputBuffer(record->videoCodec, outIndex, false);
                if (record->mIsRecording) {
                    continue;
                }
            } else if (outIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                AMediaFormat *outFormat = AMediaCodec_getOutputFormat(record->videoCodec);
                ssize_t track = AMediaMuxer_addTrack(record->mMuxer, outFormat);
                const char *s = AMediaFormat_toString(outFormat);
                record->mVideoTrack = 0;
                LOGI("video out format %s", s);
                LOGE("add video track status-->%d", track);
                if (record->mAudioTrack >= 0 && record->mVideoTrack >= 0) {
                    AMediaMuxer_start(record->mMuxer);
                }
            }
        } while (outIndex >= 0);
//        int64_t lt = systemnanotime() - time;
//        if (record->fpsTime > lt) {
//            usleep(record->sleepTime);
//        }
    }
    return 0;
}