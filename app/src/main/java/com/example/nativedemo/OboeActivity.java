package com.example.nativedemo;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.view.MotionEvent;
import android.widget.Toast;

public class OboeActivity extends AppCompatActivity {

    public native int createStream();

    public native void destroyStream();

    public native int playSound(boolean enable);


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_oboe);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if(createStream() != 0) {
            Toast.makeText(this, "occur an error",Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        destroyStream();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {

        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                playSound(true);
                break;
            case MotionEvent.ACTION_UP:
                playSound(false);
                break;

        }

        return super.onTouchEvent(event);
    }
}
























