/*
 * Copyright 2018 Rockchip Electronics Co. LTD
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

#ifndef _SKV_LIB_DEFINE_
#define _SKV_LIB_DEFINE_

#define INFO_RIGHT 1                // Succeed initialize SKV preprocessor
#define INFO_PARAM_ERROR 2          // Cann't support the parameter of audio that you input
#define INFO_EXCEEDDATE_ERROR 3     // Exceed the limited date
#define INFO_UNKNOW_ERROR 4         // Unknown error

#define FRAME_SIZE 256
#define FRAME_SIZE_PBNLMS 256
#define NUM_PBNLMS_FILTER 4

typedef enum _SkvAECEnable{
    AEC_EN_DELAY   = 1 << 0,
    AEC_EN_CHN_MAP = 1 << 1,
} SkvAecEnable;

typedef enum _SkvBFEnable {
    BF_EN_FAST_AEC = 1 << 0,
    BF_EN_WAKEUP   = 1 << 1,
    BF_EN_DEREV    = 1 << 2,
    BF_EN_NLP      = 1 << 3,
    BF_EN_AES      = 1 << 4,
    BF_EN_AGC      = 1 << 5,
    BF_EN_ANR      = 1 << 6,
    BF_EN_GSC      = 1 << 7,
    BF_GSC_METHOD  = 1 << 8,
    BF_EN_FIX      = 1 << 9,
    BF_EN_STDT     = 1 << 10,
    BF_EN_CNG      = 1 << 11,
} SkvBFEnable;

typedef enum _SkvRxEnable{
    RX_EN_ANR = 1 << 0,
} SkvRxEnable;

typedef struct _SkvAecParam {
    int      pos;
    int      drop_ref_channel;
    int      aec_mode;
    int      delay_len;
    int      look_ahead;
    short   *mic_chns_map;
} SkvAecParam;

typedef struct _SkvAnrParam {
    float    noiseFactor;
    int      swU;
    float    psiMin;
    float    psiMax;
    float    fGmin;
} SkvAnrParam;

typedef struct _SkvAgcParam {
    float    attack_time;
    float    release_time;
    float    max_gain;
    float    max_peak;
    float    fRth0;
    float    fRk0;
    float    fRth1;

    int      fs;
    int      frmlen;
    float    attenuate_time;
    float    fRth2;
    float    fRk1;
    float    fRk2;
    float    fLineGainDb;
    int      swSmL0;
    int      swSmL1;
    int      swSmL2;
} SkvAgcParam;

typedef struct _SkvDereverbParam {
    int      rlsLg;
    int      curveLg;
    int      delay;
    float    forgetting;
    float    t60;
    float    coCoeff;
} SkvDereverbParam;

typedef struct _SkvNlpParam {
    short    nlp16k[8][2];
} SkvNlpParam;

typedef struct _SkvCngParam{
    float    fGain;
    float    fMpy;
    float    fSmoothAlpha;
    float    fSpeechGain;
} SkvCngParam;

typedef struct _SkvDtdParam {
    float    ksiThd_high;
    float    ksiThd_low;
} SkvDtdParam;

typedef struct _SkvAesParam {
    float beta_up;
    float beta_down;
} SkvAesParam;

typedef struct _SkvBeamFormParam {
    int      model_en;
    int      ref_pos;
    int      targ;
    int      num_ref_channel;
    int      drop_ref_channel;

    SkvDereverbParam* dereverb;
    SkvAesParam* aes;
    SkvNlpParam* nlp;
    SkvAnrParam* anr;
    SkvAgcParam* agc;
    SkvCngParam* cng;
    SkvDtdParam* dtd;
} SkvBeamFormParam;

typedef struct _SkvRxParam {
    /* Parameters of agc */
    int model_en;
    SkvAnrParam* anr;
} SkvRxParam;

#endif  // _SKV_LIB_DEFINE_

