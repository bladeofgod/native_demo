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
















