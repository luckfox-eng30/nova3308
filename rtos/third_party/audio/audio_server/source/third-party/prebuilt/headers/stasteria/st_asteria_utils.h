/*
 * Copyright 2020 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * author: sensetime stasteria v0.1.3
 *   date: 2020-08-28
 */

#ifndef ST_ASTERIA_UTILS_H
#define ST_ASTERIA_UTILS_H
#include <string>
#include <sys/time.h>
#include <sys/timeb.h>

inline const char* time_str()
{
    static const size_t STR_LEN = 22;
    static char time_str[STR_LEN] = {0};

    struct timeb tb;
    ftime(&tb);
    struct tm *t = NULL;
    t = localtime(&tb.time);
    snprintf(time_str, STR_LEN, "%04d%02d%02d %02d:%02d:%02d.%03d",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec, tb.millitm);
    return time_str;
}

inline double time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

#define LOGE(fmt, ...)  fprintf(stderr, "[%s] asteria error: " fmt "\n", time_str(), ## __VA_ARGS__)
#define LOGI(fmt, ...)  fprintf(stdout, "[%s] asteria info : " fmt "\n", time_str(), ## __VA_ARGS__)
#define LOGD(fmt, ...)  fprintf(stdout, "[%s] asteria debug: " fmt "\n", time_str(), ## __VA_ARGS__)

#define TIME_S(tag)  double time_##tag##_start = time_ms();
#define TIME_E(tag)  double time_##tag##_end = time_ms(); LOGD(#tag " time: %.2f ms", time_##tag##_end - time_##tag##_start);

#define ST_CHECK_RET(x) { \
    if (ret != ST_OK) { \
        LOGE(x " failed: %d (%s)", ret, STAsteriaGetDescription(ret)); \
        return ret; \
    } \
}

#endif // ST_ASTERIA_UTILS_H
