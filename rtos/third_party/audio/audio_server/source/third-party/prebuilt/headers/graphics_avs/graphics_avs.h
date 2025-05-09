/****************************************************************************
*
*    Copyright (c) 2020 - 2021 by Rockchip Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Rockchip Corporation. This is proprietary information owned by
*    Rockchip Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Rockchip Corporation.
*
*****************************************************************************/

#ifndef GRAPHICS_AVS_H
#define GRAPHICS_AVS_H

typedef unsigned int uint32_t;

typedef struct RKGFX_AVS_CONTEXT_S {
    uint32_t apiVersion;
    uint32_t numView;
    char * meshFilePath;
    uint32_t inputBufferWidth;
    uint32_t inputBufferHeight;
    uint32_t inputBufferFormat;
    uint32_t inputIsAFBC;
    uint32_t outputBufferWidth;
    uint32_t outputBufferHeight;
    uint32_t outputBufferFormat;
    uint32_t outputIsAFBC;
    uint32_t useFd;
    int* inputFdArray;
    uint32_t useSync;
} RKGFX_AVS_CONTEXT_T;

typedef enum {
    RKGFX_AVS_SUCCESS,
    RKGFX_AVS_INVALID_PARAM,
    RKGFX_AVS_FAILED,
} RKGFX_AVS_STATUS;

#ifdef __cplusplus
extern "C"
{
#endif

RKGFX_AVS_STATUS RKGFX_AVS_init(void* context);
RKGFX_AVS_STATUS RKGFX_AVS_AllProcess(void* context);
RKGFX_AVS_STATUS RKGFX_AVS_deinit(void* context);
RKGFX_AVS_STATUS RKGFX_AVS_wait_sync(void* gles_proceess_sync);
void* RKGFX_AVS_create_sync(void);

#ifdef __cplusplus
}
#endif
#endif  // GRAPHICS_AVS_H