package com.quinn.amediacodecrecord;

import android.media.MediaFormat;

/**
 * Created by Administrator on 2017/8/15.
 */

public class NativeRecord {

    static {
        System.loadLibrary("NativeRecord");
    }

    public boolean recordPreper(final String path, final int width, final int height, final int frameRate) {
        return recordPreper(path, width, height, frameRate,
                Util.checkColorFormat(MediaFormat.MIMETYPE_VIDEO_AVC), Util.calcBitRate(frameRate, width, height));
    }

    public boolean recordPreper(final String path, final int width, final int height, final int frameRate, final int colorFormat, final int bitRate) {
        return nativePrepare(path, width, height, frameRate, colorFormat, bitRate);
    }

    public void sendVideoData(byte[] data){
        nativeSendVideoData(data);
    }

    public boolean start(){
        return nativeStart();
    }

    public boolean isRecording(){
        return nativeIsRecording();
    }

    public void stop(){
        nativeStop();
    }

    private static final native boolean nativePrepare(String path, int width, int height, final int frameRate, final int colorFormat, int bitRate);

    private static final native void nativeSendVideoData(byte[] data);

    private static final native boolean nativeStart();

    private static final native boolean nativeIsRecording();

    private static final native int nativeStop();
}
