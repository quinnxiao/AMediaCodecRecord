//
// Created by Administrator on 2017/8/15.
//

#ifndef INC_720VV_O2_ARGUMENTS_H
#define INC_720VV_O2_ARGUMENTS_H

#include <jni.h>

typedef struct Arguments{
    jobject mAudioRecord;
    jmethodID startRecording_id,read_id, stop_id, release_id;
    JavaVM *javaVM;
    JNIEnv *jniEnv;

    const char *path;        //文件保存的路径
    int width, height;
};

#endif //INC_720VV_O2_ARGUMENTS_H
