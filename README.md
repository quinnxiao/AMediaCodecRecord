# AMediaCodecRecord
调用底层NDKAMediaCodec.h进行音视频录制

#音频初始化方式</br>
```cpp
    jclass AudioRecordClass = arguments->jniEnv->FindClass("android/media/AudioRecord");
    jmethodID init_id = arguments->jniEnv->GetMethodID(AudioRecordClass, "<init>", "(IIIII)V");
    jmethodID minBufferSize_id = arguments->jniEnv->GetStaticMethodID(AudioRecordClass,
                                                                      "getMinBufferSize", "(III)I");
    arguments->startRecording_id = arguments->jniEnv->GetMethodID(AudioRecordClass,
                                                                  "startRecording",
                                                                  "()V");
    arguments->read_id = arguments->jniEnv->GetMethodID(AudioRecordClass, "read", "([BII)I");
    arguments->stop_id = arguments->jniEnv->GetMethodID(AudioRecordClass, "stop", "()V");
    arguments->release_id = arguments->jniEnv->GetMethodID(AudioRecordClass, "release", "()V");

    jint bufferSize =
            arguments->jniEnv->CallStaticIntMethod(AudioRecordClass, minBufferSize_id, SAMPLE_RATE,
                                                   CHANNEL_CONFIG,
                                                   AUDIO_FORMAT) * CHANNEL_COUNT;
    jobject AudioRecordObject = arguments->jniEnv->NewObject(AudioRecordClass, init_id,
                                                             AUDIO_SOURCE, SAMPLE_RATE,
                                                             CHANNEL_CONFIG,
                                                             AUDIO_FORMAT, bufferSize);
    //当方法返回后AudioRecordObject对象将被回收，所以需要NewGlobalRef方法重新new一个全局对象，再调用DeleteGlobalRef销毁
    arguments->mAudioRecord = arguments->jniEnv->NewGlobalRef(AudioRecordObject);
```
#音频采集方式</br>
```cpp
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
```
#视频采集方式</br>
```cpp
void NativeRecord::feedData(void *data) {
    nowFeedData = data;
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
```
