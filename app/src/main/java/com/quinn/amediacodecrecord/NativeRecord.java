package com.quinn.amediacodecrecord;

/**
 * Created by Administrator on 2017/8/15.
 */

public class NativeRecord {

    static {
        System.loadLibrary("NativeRecord");
    }

    public static final native boolean prepare(String path, int width, int height);

    public static final native void sendVideoData(byte[] data);

    public static final native int start();

    public static final native int stop();
}
