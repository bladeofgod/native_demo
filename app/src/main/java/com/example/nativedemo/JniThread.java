package com.example.nativedemo;

public class JniThread {

    static {
        System.loadLibrary("jnithread");
    }


    //1 一般线程
    public native void normalThread();

    //2 生产、消费者线程
    public native void mutexThread();

    //回调线程
    public native void callbackThread();

    private OnErrorListener onErrorListener;

    public void setOnErrorListener(OnErrorListener errorListener) {
        this.onErrorListener = errorListener;
    }

    //jni 调用此方法
    public void onError(int code,String msg) {
        if(onErrorListener != null) {
            onErrorListener.onError(code,msg);
        }
    }

    public interface OnErrorListener{
        void onError(int code,String msg);
    }


}




















