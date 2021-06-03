//
// Created by Admin on 2021/6/3.
//

#ifndef NATIVEDEMO_WILISTENER_H
#define NATIVEDEMO_WILISTENER_H


class WIListener {
public:
    JavaVM* jvm;
    _JNIEnv* jEnv;//native 线程 env 对象
    jobject jObj;
    jmethodID jMid;// java 方法

public:
    WIListener(JavaVM* vm,_JNIEnv* env,jobject obj);
    ~WIListener();
    void onError(int type,int code,const char* msg);
};


#endif //NATIVEDEMO_WILISTENER_H
