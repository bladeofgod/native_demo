#include <jni.h>
#include <string>
#include <pthread.h>
#include <android/log.h>

#include "oboe_sine_player.h"
#include "WIAndroidLog.h"
#include "WIListener.h"
#include "unistd.h"
#include "queue"

#include "oboe_sine_player.h"
static OboeSinePlayer * oboePlayer = nullptr ;


// Android log function wrappers
static const char* kTAG = "hello-jniCallback";
#define LOGI(...) \
  ((void)__android_log_print(ANDROID_LOG_INFO, kTAG, __VA_ARGS__))
#define LOGW(...) \
  ((void)__android_log_print(ANDROID_LOG_WARN, kTAG, __VA_ARGS__))
#define LOGE(...) \
  ((void)__android_log_print(ANDROID_LOG_ERROR, kTAG, __VA_ARGS__))

///=======================oboe demo ===================================


extern "C" {

/* Create Oboe playback stream
 * Returns:  0 - success
 *          -1 - failed
 */
JNIEXPORT jint JNICALL Java_com_example_nativedemo_OboeActivity_createStream(
        JNIEnv *env,jobject instance
        ) {
    //new 是创建在堆上的
    oboePlayer = new OboeSinePlayer();
    return oboePlayer ? 0 : -1;
}

JNIEXPORT void JNICALL Java_com_example_nativedemo_OboeActivity_destroyStream(
        JNIEnv *env,jobject instance) {
    if(oboePlayer) {
        delete oboePlayer;
        oboePlayer = nullptr;
    }
}

/*
 * Play sound with pre-created Oboe stream
 * returns:  0  - success
 *          -1  - failed (stream has not created yet )
 */

JNIEXPORT jint JNICALL Java_com_example_nativedemo_OboeActivity_playSound(
        JNIEnv *env, jobject instance, jboolean enable) {
    jint result = 0;
    if(oboePlayer) {
        oboePlayer->enable(enable);
    } else {
        result = -1;
    }
    return result;
}



}



///======================Jni-thread-demo=================================

/*
    pthread 文档：
1、pthread_t :用于声明一个线程对象如：pthread_t thread;
2、pthread_creat :用于创建一个实际的线程如：pthread_create(&pthread,NULL,threadCallBack,NULL);其总共接收4个参数，第一个参数为pthread_t对象，第二个参数为线程的一些属性我们一般传NULL就行，第三个参数为线程执行的函数（ void* threadCallBack(void *data) ），第四个参数是传递给线程的参数是void*类型的既可以传任意类型。
3、pthread_exit :用于退出线程如：pthread_exit(&thread)，参数也可以传NULL。注：线程回调函数最后必须调用此方法，不然APP会退出（挂掉）。
4、pthread_mutex_t :用于创建线程锁对象如：pthread_mutex_t mutex;
5、pthread_mutex_init :用于初始化pthread_mutex_t锁对象如：pthread_mutex_init(&mutex, NULL);
6、pthread_mutex_destroy :用于销毁pthread_mutex_t锁对象如：pthread_mutex_destroy(&mutex);
7、pthread_cond_t :用于创建线程条件对象如：pthread_cond_t cond;
8、pthread_cond_init :用于初始化pthread_cond_t条件对象如：pthread_cond_init(&cond, NULL);
9、pthread_cond_destroy :用于销毁pthread_cond_t条件对象如：pthread_cond_destroy(&cond);
10、pthread_mutex_lock :用于上锁mutex，本线程上锁后的其他变量是不能被别的线程操作的如：pthread_mutex_lock(&mutex);
11、pthread_mutex_unlock :用于解锁mutex，解锁后的其他变量可以被其他线程操作如：pthread_mutex_unlock(&mutex);
12、pthread_cond_signal :用于发出条件信号如：pthread_cond_signal(&mutex, &cond);
13、pthread_cond_wait :用于线程阻塞等待，直到pthread_cond_signal发出条件信号后才执行退出线程阻塞执行后面的操作。
*/

JavaVM* jvm;

//--------------normal thread-------------------
pthread_t pthread;
void* threadDoThings(void* data) {
    LOGD("jni thread do things...");
    pthread_exit(&pthread);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_nativedemo_JniThread_normalThread(JNIEnv *env,jobject instance) {
    LOGD("normal thread");
    //创建线程
    pthread_create(&pthread,NULL,threadDoThings,NULL);
}

//----------------thread lock--------------------
std::queue<int> queue;//product queue
pthread_t pthread_product;// product thread
pthread_t pthread_consumer;// consumer thread
pthread_mutex_t mutex;//thread lock
pthread_cond_t cond;// condition of lock

void* productThread(void* data) {
    while(queue.size() < 50) {
        //start product
        LOGD("make a product...");
        //加锁，确保安全
        pthread_mutex_lock(&mutex);
        queue.push(1);
        if(queue.size() > 0){
            LOGD("maker notify consumer that a product has been made : %d",queue.size());
            //notify consumer thread(other wise will block consumer thread...)
            pthread_cond_signal(&cond);
        }
        //解锁
        pthread_mutex_unlock(&mutex);
        sleep(4);//unit : second
    }
    pthread_exit(&pthread_product);
}

void *customerThread(void *data) {
    char *prod = (char *)data;
    LOGD("%",prod);
    while (1){
        pthread_mutex_lock(&mutex);//对queue操作前 加锁
        if(queue.size() > 0){
            queue.pop();
            LOGE("consumer has consume a product...standby for re-product...");
        } else {
            LOGE("no product for consume , standby for re-product...");
            pthread_cond_wait(&cond,&mutex);//阻塞当前线程，等待生产线程
        }
        pthread_mutex_unlock(&mutex);//释放锁
        usleep(500*1000);//单位 微秒  -> 0.5秒
    }
    pthread_exit(&pthread_consumer);
}



void initMutex() {
    pthread_mutex_init(&mutex,NULL);//init lock
    //pthread_mutex_destroy(&mutex);//destroy a lock
    pthread_cond_init(&cond,NULL);//初始化条件变量
    //productThread 入口函数
    pthread_create(&pthread_product,NULL,productThread,(void*)"product");
    pthread_create(&pthread_consumer,NULL,customerThread,NULL);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_nativedemo_JniThread_mutexThread(JNIEnv *env,jobject instance){
    for(int i=0;i<8;i++) {
        queue.push(i);
    }
    initMutex();
}


//回调方法
pthread_t callbackThread;

void *callbackT(void* data) {
    //获取listener指针
    WIListener *wiListener = (WIListener *)data;
    //在子线程中调用回调方法
    wiListener->onError(1,200,"child thread running success!");
    pthread_exit(&callbackThread);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_nativedemo_JniThread_callbackThread(JNIEnv *env,jobject instance) {
    WIListener* wiListener = new WIListener(jvm,env,env->NewGlobalRef(instance));
    //主线程调用java方法
    wiListener->onError(0,100,"JNIENV thread running success");
    //开启子线程  callbackT入口函数, wiListener入口函数的参数
    pthread_create(&callbackThread,NULL,callbackT,wiListener);
}





///====================MainActivity(clock demo)==============================

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
    jvm = vm;

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

    }

    /*..省略代码..*/
    (*javaVm).DetachCurrentThread();
    return context;


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

    g_ctx.mainActivityClz = (jclass) (*env).NewGlobalRef(clz);
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













