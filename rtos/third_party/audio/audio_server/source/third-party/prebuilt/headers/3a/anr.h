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
 */

#ifndef _RK_3A_ANR_INTERFACE_H_
#define _RK_3A_ANR_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Desc: init the anr lib
 * Parameters:
 *    sampleRate : the sampleRate of input data, only support 8K or 16K
 *    swFrmLen : the samples process one time, support 16ms and 20ms
 *               swFrmLen = sampleRate*0.016 or sampleRate*0.020
 *    degree : degree of noise suppression
 *             value: 0 ~ 1,set 0.9 for default
 * Return: the handle of anr lib
 */
extern void * RK_VOICE_Init(int sampleRate, int swFrmLen, float degree);

/*
 * Desc: denoise one channels
 * Parameters:
 *    handle: the handle to process data, which is init by function RK_VOICE_Init
 *    pshwIn: the input datas
 *    pshwOut: the output datas
 *    swFrmLen: the frames will be process, which is fixed to 256
 */
extern void RK_VOICE_ProcessRx(void* handle, short* pshwIn,
                                     short* pshwOut, int swFrmLen);

/*
 * Desc: destory the handle
 * Parameters:
 *    handle : the handle init by function RK_VOICE_Init
 */
extern void RK_VOICE_Destory(void * handle);

#ifdef __cplusplus
}
#endif

#endif  // _RK_3A_ANR_INTERFACE_H_