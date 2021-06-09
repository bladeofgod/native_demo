//
// Created by Admin on 2021/6/3.
//

#include <jni.h>
#include "WIListener.h"
#include "WIAndroidLog.h"
#include "log_formatter.h"


WIListener::WIListener(JavaVM *vm, _JNIEnv *env, jobject obj) {
    jvm = vm;
    jEnv = env;
    jObj = obj;
    jclass clz = env->GetObjectClass(jObj);
    if(!clz){
        LOGE("get class wrong");
        return;
    }

    //对应 jniThread中的onError方法,注意签名要一致
    // I = int   Ljava/lang/String = string   V = 返回void
    // 即 (int code , String msg)
    jMid = env->GetMethodID(clz,"onError","(ILjava/lang/String;)V");
    if(!jMid) {
        LOGE("get jmethodID wrong");
        return;
    }

}

WIListener::~WIListener() {
    delete jvm;
    delete jEnv;
    delete jObj;
    jMid = nullptr;
}

/*
 * @param type 0:env线程,   1: 子线程
 *
 *
 */

void WIListener::onError(int type, int code, const char *msg) {
    if(type == 0) {
        jstring jmsg = jEnv->NewStringUTF(msg);
        //对象，方法，参数...
        jEnv->CallVoidMethod(jObj,jMid,code,jmsg);
        jEnv->DeleteLocalRef(jmsg);
    }
    else if(type == 1) {
        JNIEnv* env;
        //子线程中获取 jnienv
        jvm->AttachCurrentThread(&env,0);

        jstring jmsg = env->NewStringUTF(msg);
        env->CallVoidMethod(jObj,jMid,code,jmsg);
        env->DeleteLocalRef(jmsg);
        //确保线程可以正常退出
        jvm->DetachCurrentThread();
    }
}

























