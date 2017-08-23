package com.quinn.amediacodecrecord;

import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build;
import android.util.Log;

import java.util.Arrays;

/**
 * Created by Administrator on 2017/8/21.
 */

public class Util {

    private static final boolean DEBUG = true;

    public static int checkColorFormat(String mime) {
        if (Build.MODEL.equals("HUAWEI P6-C00")) {
            return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible;
        }
        MediaCodecInfo[] codecList = new MediaCodecList(MediaCodecList.ALL_CODECS).getCodecInfos();
        for (MediaCodecInfo info : codecList) {
            if (info.isEncoder()) {
                String[] types = info.getSupportedTypes();
                for (String type : types) {
                    if (type.equals(mime)) {
                        if (DEBUG)
                            Log.d("YUV", "checkColorFormat(Util.java:26) type-->" + type);
                        MediaCodecInfo.CodecCapabilities c = info.getCapabilitiesForType(type);
                        if (DEBUG)
                            Log.d("YUV", "checkColorFormat(Util.java:29) color-->" + Arrays.toString(c.colorFormats));
                        for (int format : c.colorFormats) {
                            if (format == MediaCodecInfo.CodecCapabilities
                                    .COLOR_FormatYUV420Flexible) {
                                return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible;
                            }
                        }
                    }
                }
            }
        }
        return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible;
    }

    public static int calcBitRate(int frameRate, int width, int height) {
        final int bitrate = (int) (0.25f * frameRate * width * height);
        return bitrate;
    }
}
