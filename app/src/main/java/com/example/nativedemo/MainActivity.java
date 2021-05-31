package com.example.nativedemo;

import androidx.annotation.Keep;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.example.nativedemo.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    int second;
    int minute;
    int hour;
    TextView tv;

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        tv = binding.sampleText;
        tv.setText(stringFromJNI());
    }

    @Override
    protected void onResume() {
        super.onResume();
        hour = minute = second = 0;
        startTicks();
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopTicks();;
    }

    @Keep
    private void updateTimer() {
        ++second;
        if(second >= 60) {
            ++minute;
            second -=60;
            if(minute >= 60) {
                ++hour;
                minute -= 60;
            }
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String ticks = "" + MainActivity.this.hour + ":" +
                        MainActivity.this.minute + ":" +
                        MainActivity.this.second;
                MainActivity.this.tv.setText(ticks);
            }
        });
    }


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    public native void startTicks();
    public native void stopTicks();
}