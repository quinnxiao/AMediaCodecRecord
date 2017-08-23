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

    public static int checkColorFormat(String mime) {
        if (Build.MODEL.equals("HUAWEI P6-C00")) {
            return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;
        }
        for (int i = 0; i < MediaCodecList.getCodecCount(); i++) {
            MediaCodecInfo info = MediaCodecList.getCodecInfoAt(i);
            if (info.isEncoder()) {
                String[] types = info.getSupportedTypes();
                for (String type : types) {
                    if (type.equals(mime)) {
                        Log.e("YUV", "type-->" + type);
                        MediaCodecInfo.CodecCapabilities c = info.getCapabilitiesForType(type);
                        Log.e("YUV", "color-->" + Arrays.toString(c.colorFormats));
                        for (int j = 0; j < c.colorFormats.length; j++) {
                            if (c.colorFormats[j] == MediaCodecInfo.CodecCapabilities
                                    .COLOR_FormatYUV420Planar) {
                                return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Planar;
                            } else if (c.colorFormats[j] == MediaCodecInfo.CodecCapabilities
                                    .COLOR_FormatYUV420SemiPlanar) {
                                return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;
                            }
                        }
                    }
                }
            }
        }
        return MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420SemiPlanar;
    }

    public static int calcBitRate(int frameRate,int width,int height){
        final int bitrate = (int) (0.25f * frameRate * width * height);
        return bitrate;
    }
}
