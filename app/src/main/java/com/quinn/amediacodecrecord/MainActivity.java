package com.quinn.amediacodecrecord;

import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;

import java.io.IOException;

public class MainActivity extends AppCompatActivity {

    private Button button;
    private TextureView textureView;

    private NativeRecord record;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        record = new NativeRecord();

        button = (Button) findViewById(R.id.btn);
        textureView = (TextureView) findViewById(R.id.surface);
        textureView.setSurfaceTextureListener(listener);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (!record.isRecording()) {
                    if (record.recordPreper("/sdcard/720VV_O2/local/temp.mp4",1920,1080,30)){
                        record.start();
                        button.setText("Stop");
                    }
                } else {
                    record.stop();
                    button.setText("Start");
                }
            }
        });
    }

    private Camera camera;
    private TextureView.SurfaceTextureListener listener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            camera = Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK);
            Camera.Parameters parameters = camera.getParameters();
            parameters.setRotation(90);
            parameters.setPictureSize(1920,1080);
            camera.setParameters(parameters);
            try {
                camera.setPreviewTexture(surface);
            } catch (IOException e) {
                e.printStackTrace();
            }
            camera.setPreviewCallback(previewCallback);
            camera.startPreview();
        }

        @Override
        public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
        }

        @Override
        public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
            if (camera!=null) {
                camera.stopPreview();
                camera.setPreviewCallback(null);
                camera.release();
                camera = null;
            }
            return false;
        }

        @Override
        public void onSurfaceTextureUpdated(SurfaceTexture surface) {

        }
    };

    private Camera.PreviewCallback previewCallback = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(final byte[] data, Camera camera) {
            if (record.isRecording())
                record.sendVideoData(data);
        }
    };
}
