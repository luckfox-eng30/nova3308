#ifndef _DSD2PCM_CONV_H_INCLUDED
#define _DSD2PCM_CONV_H_INCLUDED

#include <stdint.h>
#include "dsd_common.h"
#include "conststr.h"

#define FIFOSIZE 64                  /* must be a power of two */
#define FIFOMASK (FIFOSIZE - 1)

typedef float real_t;

typedef struct {
    uint8_t fifo[FIFOSIZE];
    uint32_t  fifopos;
} dsd2pcm_channel_t;

typedef struct {
    int               channels;
    int               dsd_samplerate;
    int               pcm_samplerate;
    int               decimation;//抽取因子
    real_t            gain;//增益
    dsd2pcm_channel_t ch[MAX_CHANNELS];

    int ctables[256][60];
    int preinit;
    int ctable_len;
} dsd2pcm_converter_t;

#ifdef __cplusplus
extern "C"
{
#endif

void *dsd2pcm_converter_init(dec_type type, int channels, int dsd_samplerate, int pcm_samplerate);
void dsd2pcm_converter_deinit(dsd2pcm_converter_t* dsd2pcm);
int dsd2pcm_converter_convert(dsd2pcm_converter_t* dsd2pcm, uint8_t* dsd_data, uint32_t dsd_size,  uint8_t* pcm_out , uint16_t bps, int dsf);
void dsd2pcm_converter_set_gain(dsd2pcm_converter_t* dsd2pcm, real_t dB_gain);
void dsd2pcm_converter_save(dsd2pcm_converter_t* dsd2pcm);

#ifdef __cplusplus
}
#endif

#endif

