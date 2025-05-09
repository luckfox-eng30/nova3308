#ifndef __DSF_DEC_H__
#define __DSF_DEC_H__

#include "AudioConfig.h"
#include "dsd_common.h"

typedef int(*diff_read)(void *self, char *buf, uint32_t size);
typedef int(*diff_write)(void *self, char *buf, uint32_t size);

struct diff
{
    /* FVER tag */
    uint32_t version;

    /* PROP tag */
    /* PROP-FS tag */
    uint32_t sample_rate;
    /* PROP-CHNL tag */
    uint16_t channels;
    /* PROP-CMPR tag */
    uint32_t compression_type;

    /* data */
    uint32_t sample_bytes;

    uint32_t frames;
    uint32_t frame_rate;
    uint32_t frame_size;

    dec_type type;
    int raw_dsd;
    int out_ch;
    int out_rate;

    diff_read read;
    diff_write write;

    void *userdata;
};

int diff_open(struct diff *diff);
int diff_process(struct diff *diff);

#endif

