#include <jni.h>
#include <string>
#include <pthread.h>
#include <android/log.h>

// Android log function wrappers
static const char* kTAG = "hello-jniCallback";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_nativedemo_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

//结构体 别名：TickContext
typedef struct tick_context{
    JavaVM          *javaVm;//java vm
    jclass          mainActivityClz;
    jobject         mainActivityObj;
    pthread_mutex_t lock;//互斥锁
    int             done;
} TickContext;
//初始化结构体

TickContext g_ctx;

/*
 * 调用System.loadLibrary("native-lib")时，进而(最先)调用此方法
 *  只会调用一次
 *  可以藉由JNI_OnLoad()来获取JNIEnv.JNIEnv代表java环境，通过JNIEnv*指针就可以对java端的代码进行操作
 */

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm,void* reserved){
    JNIEnv* env;
    //将 (g_ctx) 内存的起始地址开始一直到结束，全部初始为0
    memset(&g_ctx,0,sizeof(g_ctx));

    //保存 vm
    g_ctx.javaVm = vm;
    /*
     * void*指针不指向任何数据类型，它属于一种未确定类型的过渡型数据，
     * 因此如果要访问实际存在的数据，必须将void*指针强转成为指定一个确定的数据类型的数据，
     * 如int*、string*等。void*指针只支持几种有限的操作：
     * 与另一个指针进行比较；向函数传递void*指针或从函数返回void*指针；
     * 给另一个void*指针赋值。不允许使用void*指针操作它所指向的对象，
     * 例如，不允许对void*指针进行解引用。不允许对void*指针进行算术操作。
     *
     * *void 允许赋值任意类型的指针
     *
     */

    //获取环境变量
    if((*vm).GetEnv((void**)&env,JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }


    /*
     * ...省略一些代码...
     */

    g_ctx.done = 0;
    g_ctx.mainActivityObj = NULL;
    return JNI_VERSION_1_6;

}

/*
 *
 *  calling back to MainActivity::
 *
 */

void* UpdateTicks(void *context){
    TickContext *pCtx = (TickContext*) context;
    JavaVM *javaVm = pCtx->javaVm;
    JNIEnv *env;
    jint res = (*javaVm).GetEnv((void**)&env,JNI_VERSION_1_6);
    if(res != JNI_OK){
        res = (*javaVm).AttachCurrentThread(&env,NULL);
        if(JNI_OK != res) {
            LOGE("Failed to AttachCurrentThread, ErrorCode = %d", res);
            return NULL;
        }
    }

    /*
     * ...省略一部分代码...
     */

    /*
     * 注意区分版本：
     * 第一个参数：JNI接口对象；
     * 第二个参数：Java类对象；
     * 第三个参数：参数名（或方法名）；
     * 第四个参数：该参数（或方法）的签名。
     *
     */

    jmethodID timerId = (*env).GetMethodID(pCtx->mainActivityClz,
                                           "updateTimer","()V");

    //时间结构体
    struct timeval beginTime,curTime,usedTime,leftTime;

    const struct timeval kOneSecond = {
            (__kernel_time_t)1,//seconds
            (__kernel_suseconds_t)0, //微秒
    };

    while(1) {
        //获取当前时间
        gettimeofday(&beginTime,NULL);
        //线程调用该函数让互斥锁上锁，如果该互斥锁已被另一个线程锁定和拥有，
        // 则调用该线程将阻塞，直到该互斥锁变为可用为止
        pthread_mutex_lock(&pCtx->lock);
        int done = pCtx->done;
        if(pCtx->done) {
            pCtx->done = 0;
        }
        //释放锁
        pthread_mutex_unlock(& pCtx->lock);
        if(done) {
            break;
        }

        //调用 updateTimer
        (*env).CallVoidMethod(pCtx->mainActivityObj,timerId);

        gettimeofday(&curTime,NULL);

        // start,stop,diff
        timersub(&curTime,&beginTime,&usedTime);
        timersub(&kOneSecond,&usedTime,&leftTime);

        //时间结构体
        struct timespec sleepTime;
        sleepTime.tv_sec = leftTime.tv_sec;//秒
        //纳秒                    //微秒
        sleepTime.tv_nsec = leftTime.tv_usec * 1000;

        if (sleepTime.tv_sec <= 1) {
            //当前线程睡眠
            nanosleep(&sleepTime,NULL);
        } else {
            //大于1秒
            /*...*/
        }
        /*..省略代码..*/
        (*javaVm).DetachCurrentThread();
        return context;

    }


}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nativedemo_MainActivity_startTicks(JNIEnv *env,jobject instance) {
    //声明一个线程
    pthread_t threadInfo_;
    //线程属性
    pthread_attr_t threadAttr_;

    //初始化一个'线程属性'对象
    //0 - 成功，非0 - 失败
    pthread_attr_init(&threadAttr_);
    /*
     * 一个可结合的线程能够被其他线程收回其资源和杀死；在被其他线程回收之前，
     * 它的存储器资源（如栈）是不释放的。相反，一个分离的线程是不能被其他线程回收或杀死的
     * ，它的存储器资源在它终止时由系统自动释放。
     */

    //设置线程 分离还是可结合状态
    //PTHREAD_CREATE_DETACHED（分离线程）和 PTHREAD _CREATE_JOINABLE（非分离线程）
    pthread_attr_setdetachstate(&threadAttr_,PTHREAD_CREATE_DETACHED);

    //初始化互斥锁
    pthread_mutex_init(&g_ctx.lock,NULL);

    jclass clz = (*env).GetObjectClass(instance);

    g_ctx.mainActivityClz = clz;
    g_ctx.mainActivityObj = (*env).NewGlobalRef(instance);

    /*
     * pthread_create是（Unix、Linux、Mac OS X）等操作系统的创建线程的函数。
     * 它的功能是创建线程（实际上就是确定调用该线程函数的入口点），在线程创建以后，就开始运行相关的线程函数。
     */
    //线程创建,并执行UpdateTicks 方法
    int result = pthread_create(&threadInfo_,&threadAttr_,UpdateTicks,&g_ctx);

    assert(result == 0);

    //销毁线程属性
    pthread_attr_destroy(&threadAttr_);

    (void)result;
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_nativedemo_MainActivity_stopTicks(JNIEnv *env,jobject instance) {
    pthread_mutex_lock(&g_ctx.lock);//开启线程锁
    g_ctx.done = 1;
    pthread_mutex_unlock(&g_ctx.lock);

    // waiting for ticking thread to flip the done flag
    struct timespec sleepTime;
    //初始化结构体内存为0
    memset(&sleepTime,0,sizeof(sleepTime));

    sleepTime.tv_nsec = 100000000;

    /*
     * UpdateTicks方法运行在子线程
     *
     * 该方法内部会重置 done 的值
     *
     */
    while (g_ctx.done) {
        //等待 done为0，
        nanosleep(&sleepTime,NULL);
    }

    // release object we allocated from StartTicks() function

    (*env).DeleteGlobalRef(g_ctx.mainActivityClz);
    (*env).DeleteGlobalRef(g_ctx.mainActivityObj);

    g_ctx.mainActivityObj = NULL;
    g_ctx.mainActivityClz = NULL;

    //销毁线程锁
    pthread_mutex_destroy(&g_ctx.lock);

}













