//
// Created by Admin on 2021/6/7.
//


#include <initializer_list>//列表初始化容器
#include <memory>
#include <cstdlib>//函数与符号常量
#include <cstring>
#include <jni.h>
#include <cerrno>
#include <cassert>

//绘制界面
#include <EGL/egl.h>
#include <GLES//gl.h>

//android 相关
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/*
 * 保存的状态
 *
 */

struct saved_state {
    float angle;
    int32_t x;
    int32_t y;
};

/*
 * 状态共享给app
 *
 */

struct engine{
    struct android_app* app;

    ASensorManager* sensorManager;
    const ASensor * accelerometerSensor;
    ASensorEventQueue * sensorEventQueue;

    int animation;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    struct saved_state state;

};

/*
 * 初始化 egl context
 * 用于显示
 */

static int engine_init_display(struct engine* engine) {
    //初始化 openGl-es 和 egl

    /*
     * 配置 egl
     * rbg 888
     */
    const EGLint attributes[] = {
            EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
            EGL_BLUE_SIZE,8,
            EGL_GREEN_SIZE,8,
            EGL_RED_SIZE,8,
            EGL_NONE
    };

    EGLint w,h,format;
    EGLint numConfigs;
    EGLConfig config = nullptr;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    //param2/param3 : major 和 minor 将被赋予当前 EGL 版本号
    eglInitialize(display, nullptr, nullptr);

    //选择一个app 配置
    eglChooseConfig(display,attributes, nullptr,0,&numConfigs);
    //智能指针、独享对象
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);

    assert(supportedConfigs);
    eglChooseConfig(display,attributes,supportedConfigs.get(),numConfigs,&numConfigs);
    assert(numConfigs);

    auto i = 0;
    for(; i < numConfigs;i++) {
        auto &cfg = supportedConfigs[i];
        EGLint r,g,b,d;
        if(eglGetConfigAttrib(display,cfg,EGL_RED_SIZE,&r)
         && eglGetConfigAttrib(display,cfg,EGL_GREEN_SIZE,&g)
         && eglGetConfigAttrib(display,cfg,EGL_BLUE_SIZE,&b)
         && eglGetConfigAttrib(display,cfg,EGL_DEPTH_SIZE,&d)
         && r ==8 && g == 8 && b == 8 && d == 8) {
            config = supportedConfigs[i];
            break;
        }
    }

    //如果没有合适的，默认选择第一个配置
    if(i == numConfigs) {
        config = supportedConfigs[0];
    }

    if(config == nullptr) {
        LOGW("unable to initialize EGLConfig");
        return -1;
    }

    eglGetConfigAttrib(display,config,EGL_NATIVE_VISUAL_ID,&format);

    surface = eglCreateWindowSurface(display,config,engine->app->window, nullptr);
    context = eglCreateContext(display,config, nullptr, nullptr);

    if(eglMakeCurrent(display,surface,surface,context) == EGL_FALSE) {
        LOGW("unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display,surface,EGL_WIDTH,&w);
    eglQuerySurface(display,surface,EGL_HEIGHT,&h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;
    engine->state.angle = 0;

    //检查 openGL
    auto opengl_info = {GL_VENDOR,GL_RENDERER,GL_VERSION,GL_EXTENSIONS};

    for(auto name : opengl_info) {
        auto info = glGetString(name);
        LOGI("opengl info : %s",info);
    }

    //初始化gl 状态
    glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);
    glDisable(GL_DEPTH_TEST);

    return 0;

}

/*
 * display中的当前帧
 *
 */

static void engine_draw_frame(struct engine * engine) {
    if(engine->display == nullptr) {
        //no display
        return;
    }

    //屏幕填充一个颜色
    glClearColor(((float )engine->state.x)/engine->width,engine->state.angle,
                 ((float )engine->state.y)/engine->height,1);

    glClear(GL_COLOR_BUFFER_BIT);

    //切换buffer
    eglSwapBuffers(engine->display,engine->surface);

}

/*
 * 剔除 egl context中当前关联的display
 */

static void engine_term_display(struct engine* engine) {
    if(engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
        if(engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display,engine->context);
        }
        if(engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display,engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animation = 0;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}


/*
 * 处理下一个输入事件
 */

static int32_t engine_handle_input(struct android_app* app,AInputEvent* event) {
    auto *engine = (struct engine*)app->userData;
    if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animation = 1;
        engine->state.x = AMotionEvent_getX(event,0);
        engine->state.y = AMotionEvent_getY(event,0);
        return 1;
    }
    return 0;
}

/*
 * 处理下一个main command
 */

static void engine_handle_cmd(struct android_app* app,int32_t cmd) {
    auto* engine = (struct engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            //系统要求存储当前state
            //给android_app->savedState(void* ptr)分配内存，大小等于saved_state
            engine->app->savedState = malloc(sizeof(struct saved_state));
            //android_app->savedState指向engine->state
            *((struct saved_state*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(struct saved_state);
            break;
        case APP_CMD_INIT_WINDOW:
            //窗口显示前的准备工作
            if(engine->app->window != nullptr) {
                engine_init_display(engine);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            //window 即将隐藏或者关闭，清理资源
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            //app 获取焦点时,开始监控
            if(engine->accelerometerSensor != nullptr) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);
                //暂定为 每秒60个事件
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor,
                                               (1000L/60)* 1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            //app 失去焦点 停止监控
            //可以避免浪费电
            if(engine->accelerometerSensor != nullptr) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }
            //停滞动画
            engine->animation = 0;
            engine_draw_frame(engine);
            break;
        default:
            break;

    }
}

/*
 * AcquireASensorManagerInstance(void)
 *    Workaround ASensorManager_getInstance() deprecation false alarm
 *    for Android-N and before, when compiling with NDK-r15
 */

/*
 * Linux在<dlfnc.h>库中提供了加载和处理动态连接库的系统调用
 * dlopen : 打开一个动态连接库，并返回一个类型为void*的handler,flag为打开模式，可选的模式有两种
RTLD_LAZY 暂缓决定，等有需要时再解出符号
RTLD_NOW 立即决定，返回前解除所有未决定的符号。
dlerror : 返回dl操作的错误，若没有出现错误，则返回NUlL，否则打印错误信息
dlsym : 查找动态链接库中的符号symbol,并返回该符号所在的地址
dlclose : 关闭动态链接库句柄
 */

#include <dlfcn.h>

//获取 传感器管理 实例
ASensorManager * AcquireASensorManagerInstance(android_app* app) {
    if(!app)
        return nullptr;
}


















