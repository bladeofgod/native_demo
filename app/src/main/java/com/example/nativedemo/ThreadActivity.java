package com.example.nativedemo;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.view.View;
import android.widget.Toast;

public class ThreadActivity extends AppCompatActivity {


    JniThread jniThread;


    Handler handler = new Handler(Looper.getMainLooper()){
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            Toast.makeText(ThreadActivity.this,(String)msg.obj,Toast.LENGTH_SHORT).show();
        }
    };




    public void normalThread() {
        jniThread.normalThread();
    }
    public void mutexThread() {
        jniThread.mutexThread();
    }
    public void jniCallback() {
        jniThread.callbackThread();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_thread);

        jniThread = new JniThread();
        jniThread.setOnErrorListener(new JniThread.OnErrorListener() {
            @Override
            public void onError(int code, String msg) {
                Message message = Message.obtain();
                message.what = code;
                message.obj = msg;
                handler.sendMessage(message);
            }
        });
        findViewById(R.id.btn_jniCallback).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                jniCallback();
            }
        });
        findViewById(R.id.btn_normalThread).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                normalThread();
            }
        });
        findViewById(R.id.btn_mutexThread).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mutexThread();
            }
        });
    }
}