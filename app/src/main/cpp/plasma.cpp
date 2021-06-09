//
// Created by Admin on 2021/6/9.
//

#include <android_native_app_glue.h>

#include <errno.h>
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>



#define DEBUG 0

#define OPTIMIZE_WRITES 1

static double now_ms() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*1000. + tv.tv_usec/1000.;
}
/* We're going to perform computations for every pixel of the target
 * bitmap. floating-point operations are very slow on ARMv5, and not
 * too bad on ARMv7 with the exception of trigonometric functions.
 *
 * For better performance on all platforms, we're going to use fixed-point
 * arithmetic and all kinds of tricks
 */

typedef int32_t  Fixed;

#define  FIXED_BITS           16
#define  FIXED_ONE            (1 << FIXED_BITS)
#define  FIXED_AVERAGE(x,y)   (((x) + (y)) >> 1)

#define  FIXED_FROM_INT(x)    ((x) << FIXED_BITS)
#define  FIXED_TO_INT(x)      ((x) >> FIXED_BITS)

#define  FIXED_FROM_FLOAT(x)  ((Fixed)((x)*FIXED_ONE))
#define  FIXED_TO_FLOAT(x)    ((x)/(1.*FIXED_ONE))

#define  FIXED_MUL(x,y)       (((int64_t)(x) * (y)) >> FIXED_BITS)
#define  FIXED_DIV(x,y)       (((int64_t)(x) * FIXED_ONE) / (y))

#define  FIXED_DIV2(x)        ((x) >> 1)
#define  FIXED_AVERAGE(x,y)   (((x) + (y)) >> 1)

#define  FIXED_FRAC(x)        ((x) & ((1 << FIXED_BITS)-1))
#define  FIXED_TRUNC(x)       ((x) & ~((1 << FIXED_BITS)-1))

#define  FIXED_FROM_INT_FLOAT(x,f)   (Fixed)((x)*(FIXED_ONE*(f)))

typedef int32_t Angle;

#define ANGLE_BITS    9

#if ANGLE_BITS < 8
#error ANGLE_BITS must be at least 8
#endif

#define ANGLE_2PI           (1 << ANGLE_BITS)
#define ANGLE_PI            (1 << (ANGLE_BITS - 1))
#define ANGLE_PI2            (1 << (ANGLE_BITS - 2))
#define ANGLE_PI4            (1 << (ANGLE_BITS - 3))


#define ANGLE_FROM_FLOAT(x) (Angle)((x)*ANGLE_PI/M_PI)
#define  ANGLE_TO_FLOAT(x)     ((x)*M_PI/ANGLE_PI)

#if ANGLE_BITS <= FIXED_BITS
#  define  ANGLE_FROM_FIXED(x)     (Angle)((x) >> (FIXED_BITS - ANGLE_BITS))
#  define  ANGLE_TO_FIXED(x)       (Fixed)((x) << (FIXED_BITS - ANGLE_BITS))
#else
#  define  ANGLE_FROM_FIXED(x)     (Angle)((x) << (ANGLE_BITS - FIXED_BITS))
#  define  ANGLE_TO_FIXED(x)       (Fixed)((x) >> (ANGLE_BITS - FIXED_BITS))
#endif

//长度19 int型数组
static Fixed angle_sin_tab[ANGLE_2PI+1];

static void init_angles() {
    int nn;
    for(nn =0 ; nn < ANGLE_2PI+1;nn++) {
        double radians = nn *M_PI / ANGLE_PI;
        angle_sin_tab[nn] = FIXED_FROM_FLOAT(sin(radians));
    }
}

static inline Fixed angle_sin(Angle a) {
    return angle_sin_tab[(uint32_t)a & (ANGLE_2PI - 1)];
}

//static inline Fixed angle_cos(Angle a) {
//    return angle_sin(a + ANGLE_PI2);
//}

static inline Fixed fixed_sin(Fixed f) {
    return angle_sin(ANGLE_FROM_FIXED(f));
}

//static inline Fixed fixed_cos(Fixed f) {
//    return angle_cos(ANGLE_FROM_FIXED(f));
//}

/*
 *
 */

#define  PALETTE_BITS   8

#define  PALETTE_SIZE   (1 << PALETTE_BITS)

#if PALETTE_BITS > FIXED_BITS
#  error PALETTE_BITS must be smaller than FIXED_BITS
#endif

static uint16_t  palette[PALETTE_SIZE];

static uint16_t make565(int red,int green,int blue) {
    return (uint16_t)(
            ((red  << 8) & 0xf800) |
            ((green << 3) & 0x07e0) |
            ((blue  >> 3) & 0x001f)
            );
}


static void init_palette() {
    int nn,mm = 0;
    //铺颜色
    for(nn = 0; nn < PALETTE_SIZE/4 ; nn++) {
        int jj = (nn-mm) * 4 * 255 /PALETTE_SIZE;
        palette[nn] = make565(255,jj,255-jj);
    }

    for( mm = nn; nn < PALETTE_SIZE/2 ; nn++){
        int jj = (nn - mm) * 4 * 255 / PALETTE_SIZE;
        palette[nn] = make565(255 - jj,255,jj);
    }

    for(mm = nn; nn< PALETTE_SIZE;nn++) {
        int jj = (nn-mm) * 4 * 255 /PALETTE_SIZE;
        palette[nn] = make565(0,255-jj,255);
    }

    for(mm = nn ; nn < PALETTE_SIZE; nn++) {
        int jj = (nn-mm) *4*255/PALETTE_SIZE;
        palette[nn] = make565(jj,0,255);
    }

}

static inline uint16_t palette_from_fixed( Fixed x){
    if( x< 0) x = -x;
    if(x >= FIXED_ONE) x = FIXED_ONE - 1;
    int idx = FIXED_FRAC(x) >> (FIXED_BITS - PALETTE_BITS);
    return palette[idx & (PALETTE_SIZE - 1)];
}

static void init_tables() {
    init_palette();
    init_angles();
}

static void fill_plasma(ANativeWindow_Buffer* buffer,double t) {
    Fixed yt1 = FIXED_FROM_FLOAT(t/1230.);
    Fixed yt2 = yt1;
    Fixed xt10 = FIXED_FROM_FLOAT(t/2300.);
    Fixed xt20 = xt10;

#define  YT1_INCR   FIXED_FROM_FLOAT(1/100.)
#define  YT2_INCR   FIXED_FROM_FLOAT(1/163.)

    void* pixels = buffer->bits;

    int yy;
    for(yy = 0; yy<buffer->height;yy++) {
        uint16_t * line =   (uint16_t*) pixels;
        Fixed       base = fixed_sin(yt1) + fixed_sin(yt2);
        Fixed       xt1 = xt10;
        Fixed       xt2 = xt20;

        yt1 += YT1_INCR;
        yt2 += YT2_INCR;

#define  XT1_INCR  FIXED_FROM_FLOAT(1/173.)
#define  XT2_INCR  FIXED_FROM_FLOAT(1/242.)

#if OPTIMIZE_WRITES
        /*
         * 优化内存写入
         * 每对像素对齐 32 位
         */
        uint16_t *  line_end = line + buffer->width;

        if(line < line_end) {
            if(((uint32_t)(uintptr_t)line & 3) != 0) {
                Fixed  ii = base + fixed_sin(xt1) + fixed_sin(xt2);

                xt1 += XT1_INCR;
                xt2 += XT2_INCR;

                line[0] = palette_from_fixed(ii>>2);
                line++;

            }

            while (line + 2 <= line_end) {
                Fixed i1 = base + fixed_sin(xt1) + fixed_sin(xt2);

                xt1 += XT1_INCR;
                xt2 += XT2_INCR;

                Fixed i2 = base + fixed_sin(xt1) + fixed_sin(xt2);

                xt1 += XT1_INCR;
                xt2 += XT2_INCR;

                uint32_t pixel = ((uint32_t)palette_from_fixed(i1>>2)<<16)
                        | (uint32_t)palette_from_fixed(i2>>2);

                ((uint32_t*)line)[0] = pixel;
                line+=2;

            }

            if(line < line_end) {
                Fixed  ii = base + fixed_sin(xt1) + fixed_sin(xt2);
                line[0] = palette_from_fixed(ii >> 2);
                line++;
            }

        }

#else /*!OPTIMIZE_WRITES*/
        int xx;
        for (xx = 0; xx < buffer->width; xx++) {

            Fixed ii = base + fixed_sin(xt1) + fixed_sin(xt2);

            xt1 += XT1_INCR;
            xt2 += XT2_INCR;

            line[xx] = palette_from_fixed(ii / 4);
        }

#endif /* !OPTIMIZE_WRITES */

        // go to next line
        pixels = (uint16_t*)pixels + buffer->stride;

    }

}

typedef struct {
    double renderTime;
    double frameTime;
} FrameStats;

#define MAX_FRAME_STATS 200
#define MAX_PERIOD_MS   1500

typedef struct {
    double firstTime;
    double lastTime;
    double frameTime;

    int firstFrame;
    int numFrames;

    FrameStats frames[MAX_FRAME_STATS];

} Stats;

static void stats_init(Stats * s) {
    s->lastTime = now_ms();
    s->firstTime = 0.;
    s->firstFrame = 0;
    s->numFrames = 0;
}

static void stats_StartFrame(Stats* s) {
    s->frameTime = now_ms();
}

static void stats_endFrame(Stats* s) {
    double now = now_ms();
    double renderTime = now - s->frameTime;
    double frameTime = now - s->lastTime;
    int nn;

    if(now - s->firstTime >= MAX_PERIOD_MS) {
        if(s->numFrames > 0) {
            double minRender,maxRender,avgRender;
            double minFrame,maxFrame,avgFrame;

            int count;

            nn = s->firstFrame;

            minRender = maxRender = avgRender = s->frames[nn].renderTime;
            minFrame = maxFrame = avgFrame = s->frames[nn].frameTime;

            for(count = s->numFrames;count > 0; count-- ) {
                nn += 1;
                //reset
                if(nn >= MAX_FRAME_STATS)
                    nn -= MAX_FRAME_STATS;

                double render = s->frames[nn].renderTime;

                if(render < minRender) minRender = render;
                if(render > maxRender) maxRender = render;

                double frame = s->frames[nn].frameTime;
                if(frame < minFrame) minFrame = frame;
                if(frame > maxFrame) maxFrame = frame;

                avgRender += render;
                avgFrame += frame;

            }

            avgRender /= s->numFrames;
            avgFrame /= s->numFrames;
//            LOGI("frame/s (avg,min,max) = (%.1f,%.1f,%.1f) "
//                 "render time ms (avg,min,max) = (%.1f,%.1f,%.1f)\n",
//                 1000./avgFrame, 1000./maxFrame, 1000./minFrame,
//                 avgRender, minRender, maxRender);

        }
        s->numFrames = 0;
        s->firstFrame = 0;
        s->frameTime = now;
    }
    nn = s->firstFrame + s->numFrames;
    if (nn >= MAX_FRAME_STATS)
        nn -= MAX_FRAME_STATS;

    s->frames[nn].renderTime = renderTime;
    s->frames[nn].frameTime  = frameTime;

    if (s->numFrames < MAX_FRAME_STATS) {
        s->numFrames += 1;
    } else {
        s->firstFrame += 1;
        if (s->firstFrame >= MAX_FRAME_STATS)
            s->firstFrame -= MAX_FRAME_STATS;
    }

    s->lastTime = now;

}

//================ activity entry point ===============

struct engine {
    struct android_app* app;
    Stats  stats;
    int animating;
};

static int64_t start_ms;
static void engine_draw_frame(struct engine* engine) {
    if(engine->app->window == NULL) {
        return;
    }

    ANativeWindow_Buffer buffer;
    if(ANativeWindow_lock(engine->app->window,&buffer,NULL) < 0) {
        //LOGW("Unable to lock window buffer");
        return;
    }

    stats_StartFrame(&engine->stats);

    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC,&now);
    int64_t time_ms = (((int64_t)now.tv_sec)*1000000000LL + now.tv_nsec)/1000000;
    time_ms -= start_ms;

    //填充plasma
    fill_plasma(&buffer,time_ms);
    ANativeWindow_unlockAndPost(engine->app->window);

    stats_endFrame(&engine->stats);

}

static void engine_term_display (struct engine* engine){
    engine->animating = 0;
}

static int32_t  engine_handle_input(struct android_app* app, AInputEvent* event) {
    struct engine* engine = (struct engine*) app->userData;
    if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->animating = 1;
        return 1;
    } else if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
//        LOGI("Key event: action=%d keyCode=%d metaState=0x%x",
//             AKeyEvent_getAction(event),
//             AKeyEvent_getKeyCode(event),
//             AKeyEvent_getMetaState(event));
    }

    return 0;
}

static void engine_handle_cmd(struct android_app* app,int32_t cmd) {
    static int32_t  format = WINDOW_FORMAT_RGB_565;
    struct engine* engine = (struct  engine*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if(engine->app->window != NULL) {
                format = ANativeWindow_getFormat(app->window);
                ANativeWindow_setBuffersGeometry(app->window
                    ,ANativeWindow_getWidth(app->window)
                    ,ANativeWindow_getHeight(app->window),
                    WINDOW_FORMAT_RGB_565);
                engine_draw_frame(engine);
            }
            break;
        case APP_CMD_TERM_WINDOW:
            engine_term_display(engine);
            ANativeWindow_setBuffersGeometry(app->window,
                                             ANativeWindow_getWidth(app->window),
                                             ANativeWindow_getHeight(app->window),
                                             format);
            break;
        case APP_CMD_LOST_FOCUS:
            engine->animating = 0;
            engine_draw_frame(engine);
            break;

    }

}

void android_main(struct android_app* state) {
    static int init;

    struct engine engine;

    memset(&engine,0,sizeof(engine));

    state->userData = &engine;
    //回调
    state->onAppCmd = engine_handle_cmd;
    state->onInputEvent = engine_handle_input;

    engine.app = state;

    if(!init) {
        init_tables();
        init = 1;
    }

    struct timespec now;
    clock_gettime(CLOCKS_MONO,&now);
    start_ms = (((int64_t)now.tv_sec)*1000000000LL + now.tv_nsec)/1000000;

    stats_init(&engine.stats);

    //looper
    while (1) {
        int ident;
        int events;
        struct android_poll_source* source;

        while ((ident = ALooper_pollAll(engine.animating ? 0 : -1,
                                        NULL,&events,(void**)&source)) >= 0) {
            //处理事件
            if(source != NULL) {
                source->process(state,source);
            }

            if(state->destroyRequested != 0) {
//                LOGI("Engine thread destroy requested!");
                engine_term_display(&engine);
                return;
            }

        }

        if(engine.animating) {
            engine_draw_frame(&engine);
        }

    }

}













