package com.quinn.amediacodecrecord;

import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.media.MediaScannerConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.view.TextureView;
import android.view.View;
import android.widget.Button;

import java.io.File;
import java.io.IOException;

public class MainActivity extends AppCompatActivity {

    private static final String BASE_PATH = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES).getPath() + File.separatorChar;
    private static final String POSTFIX = ".mp4";

    private Button button;
    private TextureView textureView;

    private Camera camera;

    private NativeRecord record;
    private String path;
    private MediaScannerConnection msc;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        init();

        initView();
    }

    private void init() {
        record = new NativeRecord();
        msc = new MediaScannerConnection(this, client);
    }

    private void initView() {
        button = (Button) findViewById(R.id.btn);
        textureView = (TextureView) findViewById(R.id.surface);
        textureView.setSurfaceTextureListener(surfaceTextureListener);
        button.setOnClickListener(onClickListener);
    }

    private MediaScannerConnection.MediaScannerConnectionClient client = new MediaScannerConnection.MediaScannerConnectionClient() {
        @Override
        public void onMediaScannerConnected() {
            if (!path.isEmpty()&&msc.isConnected())
                msc.scanFile(path, null);
        }

        @Override
        public void onScanCompleted(String path, Uri uri) {
            msc.disconnect();
        }
    };

    private View.OnClickListener onClickListener = new View.OnClickListener() {
        @Override
        public void onClick(View v) {
            if (!record.isRecording()) {
                path = BASE_PATH + System.currentTimeMillis() + POSTFIX;
                if (record.recordPreper(path, 1920, 1080, 30)) {
                    record.start();
                    button.setText(R.string.stop);
                }
            } else {
                record.stop();
                button.setText(R.string.start);
                new Handler().postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        msc.connect();
                    }
                }, 1000);
            }
        }
    };

    private TextureView.SurfaceTextureListener surfaceTextureListener = new TextureView.SurfaceTextureListener() {
        @Override
        public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
            camera = Camera.open(Camera.CameraInfo.CAMERA_FACING_BACK);
            Camera.Parameters parameters = camera.getParameters();
            parameters.setPictureSize(1920, 1080);
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
            if (camera != null) {
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
