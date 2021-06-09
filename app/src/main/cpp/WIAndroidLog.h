//
// Created by Admin on 2021/6/3.
//

#pragma once
#ifndef NATIVEDEMO_WIANDROIDLOG_H
#define NATIVEDEMO_WIANDROIDLOG_H

#include <android/log.h>

#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG,"ljq1990",FORMAT,##__VA_ARGS__);
//#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,"ljq1990",FORMAT,##__VA_ARGS__);

#endif //NATIVEDEMO_WIANDROIDLOG_H




