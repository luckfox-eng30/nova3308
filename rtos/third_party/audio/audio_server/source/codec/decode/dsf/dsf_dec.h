#ifndef __DSF_DEC_H__
#define __DSF_DEC_H__

#include "AudioConfig.h"
#include "dsd_common.h"

typedef int(*dsf_read)(void *self, char *buf, uint32_t size);
typedef int(*dsf_write)(void *self, char *buf, uint32_t size);

struct dsf
{
    /* DSD tag */
    uint32_t filesize;
    uint32_t id3_pos;

    /* fmt tag */
    uint32_t format_version;
    uint32_t format_id;
    uint32_t channel_type;
    uint32_t channels;
    uint32_t frequency;
    uint32_t bits;
    uint32_t samples;
    uint32_t block_size;
    uint32_t reserved;

    /* data */
    uint32_t sample_bytes;

    uint32_t frames;
    uint32_t frames_out;

    dec_type type;
    int out_ch;
    int out_rate;

    dsf_read read;
    dsf_write write;

    void *userdata;
};

int dsf_open(struct dsf *dsf);
int dsf_process(struct dsf *dsf);

#endif

