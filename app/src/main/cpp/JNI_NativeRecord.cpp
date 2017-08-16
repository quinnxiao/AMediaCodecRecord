#include <jni.h>
#include "NativeRecord.h"

NativeRecord *record;

extern "C" {
jboolean
Java_com_quinn_amediacodecrecord_NativeRecord_prepare(JNIEnv *env, jclass type, jstring _path,
                                                      jint width,
                                                      jint height) {
    Arguments *arguments = (Arguments *) malloc(sizeof(Arguments));
    arguments->jniEnv = env;
    arguments->jniEnv->GetJavaVM(&arguments->javaVM);

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

    const char *path = env->GetStringUTFChars(_path, NULL);
    arguments->path = path;
    arguments->width = width;
    arguments->height = height;

    record = new NativeRecord(arguments);
    if (record->prepare()) {
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_quinn_amediacodecrecord_NativeRecord_sendVideoData(JNIEnv *env, jclass type,
                                                            jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    if (record != NULL) {
        record->feedData(data);
    }

    env->ReleaseByteArrayElements(data_, data, 0);
}

jint Java_com_quinn_amediacodecrecord_NativeRecord_start(JNIEnv *env, jclass type) {
    if (record != NULL) {
        record->start();
    }
    return 0;
}

void Java_com_quinn_amediacodecrecord_NativeRecord_stop(JNIEnv *env, jclass type) {
    if (record != NULL) {
        record->stop();
        record = NULL;
    }
}

}