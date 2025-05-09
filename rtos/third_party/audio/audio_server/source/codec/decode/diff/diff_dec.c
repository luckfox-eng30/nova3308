#include "diff_dec.h"

#include "dsd2pcm_conv.h"

#define CHUNK(a, b, c, d)   (((a & 0xFF) << 24) | ((b & 0xFF) << 16) | ((c & 0xFF) << 8) | (d & 0xFF))
#define FVER                CHUNK('F', 'V', 'E', 'R')
#define PROP                CHUNK('P', 'R', 'O', 'P')
#define DSD                 CHUNK('D', 'S', 'D', ' ')
#define DST                 CHUNK('D', 'S', 'T', ' ')
#define FS                  CHUNK('F', 'S', ' ', ' ')
#define CHNL                CHUNK('C', 'H', 'N', 'L')
#define CMPR                CHUNK('C', 'M', 'P', 'R')
#define LSCO                CHUNK('L', 'S', 'C', 'O')

static void diff_parse_prop(struct diff *diff, struct dsd_buf *ck_data)
{
    uint8_t *buf = ck_data->buf;
    int pos = 0;

    if (!chunk_id_is((char *)buf, "SND "))
    {
        printf("cannot find SND \n");
        return;
    }
    buf += 4;
    pos += 4;

    while (pos < ck_data->size)
    {
        switch (data_be32(buf))
        {
        case FS:
            diff->sample_rate = data_be32(buf + sizeof(struct chunk));
            break;
        case CHNL:
            diff->channels = data_be16(buf + sizeof(struct chunk));
            break;
        case CMPR:
            if (data_be32(buf + sizeof(struct chunk)) == DSD)
                diff->raw_dsd = 1;
            else
                diff->raw_dsd = 0;
            break;
        default:
            break;
        }
        pos += data_be64(buf + 4) + sizeof(struct chunk);
        buf += data_be64(buf + 4) + sizeof(struct chunk);
    }
}

int diff_open(struct diff *diff)
{
    struct chunk ck;
    struct dsd_buf ck_data = {NULL, 0, 0};
    int eos = 0;

    if (NULL == resize_dsd_buf(&ck_data, 1024))
    {
        printf("resize to 1024 failed\n");
        return -1;
    }

    /* Read FRM8 tag */
    diff->read(diff, (char *)&ck, sizeof(ck));
    if (!chunk_id_is((char *)ck.id, "FRM8"))
    {
        printf("cannot find FRM8\n");
        return -2;
    }
    if (NULL == resize_dsd_buf(&ck_data, 4))
    {
        printf("%s %d resize to 4 failed\n", __func__, __LINE__);
        return -1;
    }
    if (diff->read(diff, (char *)ck_data.buf, ck_data.size) != ck_data.size)
    {
        printf("%s %d read failed\n", __func__, __LINE__);
        return -2;
    }
    if (!chunk_id_is((char *)ck_data.buf, "DSD "))
    {
        printf("cannot find DSD \n");
        return -2;
    }

    do
    {
        if (diff->read(diff, (char *)&ck, sizeof(ck)) != sizeof(ck))
            return -2;
        switch (data_be32(ck.id))
        {
        case FVER:
            if (NULL == resize_dsd_buf(&ck_data, data_be64(ck.size)))
                return -1;
            if (diff->read(diff, (char *)ck_data.buf, ck_data.size) != ck_data.size)
                return -2;
            diff->version = data_be32(ck_data.buf);
            break;
        case PROP:
            if (NULL == resize_dsd_buf(&ck_data, data_be64(ck.size)))
                return -1;
            if (diff->read(diff, (char *)ck_data.buf, ck_data.size) != ck_data.size)
                return -2;
            diff_parse_prop(diff, &ck_data);
            break;
        case DSD:
            diff->sample_bytes = data_be64(ck.size);
            diff->frame_rate = 75;
            diff->frame_size = 9408;
            diff->frames = diff->sample_bytes / diff->frame_size;
            eos = 1;
            break;
        case DST:
            if (diff->read(diff, (char *)&ck, sizeof(ck)) != sizeof(ck))
                return -2;
            if (!chunk_id_is((char *)ck.id, "FRTE"))
                return -1;
            if (NULL == resize_dsd_buf(&ck_data, data_be64(ck.size)))
                return -1;
            if (diff->read(diff, (char *)ck_data.buf, ck_data.size) != ck_data.size)
                return -2;
            diff->frames = data_be32(ck_data.buf);
            diff->frame_rate = data_be16(ck_data.buf + 4);
            diff->frame_size = 5120;
            eos = 1;
            break;
        }
    }
    while (eos == 0);

    resize_dsd_buf(&ck_data, 0);

    diff->out_ch = diff->channels;

    switch (diff->sample_rate)
    {
    case 2822400:
        diff->type = DSD64;
        diff->out_rate = 44100;
        break;
    case 5644800:
        diff->type = DSD128;
        diff->out_rate = 88200;
        break;
    case 11289600:
        diff->type = DSD256;
        diff->out_rate = 44100;
        break;
    default:
        diff->type = DSD64;
        diff->out_rate = 44100;
        break;
    }

    return 0;
}

int diff_process(struct diff *diff)
{
    void *dsd2pcm;
    uint8_t *out_src;
    uint8_t *in_src;
    uint8_t *out;
    uint8_t *in;
    uint32_t out_size;
    uint32_t frame_size;
    int read_ret, write_ret, ret = RK_AUDIO_FAILURE;

    if (!diff->raw_dsd)
    {
        RK_AUDIO_LOG_E("Not support DST yet");
        return RK_AUDIO_FAILURE;
    }
    frame_size = diff->frame_size;
    out_size = frame_size / (diff->sample_rate / diff->out_rate >> 3) * sizeof(uint32_t);
    out_src = audio_malloc(RK_AUDIO_ALIGN(out_size, 128) + 128);
    if (!out_src)
    {
        RK_AUDIO_LOG_E("malloc %ld failed", out_size);
        goto out_err;
    }
    out = (uint8_t *)RK_AUDIO_ALIGN((uint32_t)out_src, 128);
    in_src = audio_malloc(RK_AUDIO_ALIGN(frame_size, 128) + 128);
    if (!in_src)
    {
        RK_AUDIO_LOG_E("malloc %ld failed", frame_size);
        goto in_err;
    }
    in = (uint8_t *)RK_AUDIO_ALIGN((uint32_t)in_src, 128);
    dsd2pcm = dsd2pcm_converter_init(diff->type, diff->channels, diff->sample_rate, diff->out_rate);
    if (!dsd2pcm)
    {
        RK_AUDIO_LOG_E("create dsd2pcm failed");
        goto dsd_err;
    }
    RK_AUDIO_LOG_V("%ld [%ld] => %ld [%d]\n", frame_size, diff->sample_rate, out_size, diff->out_rate);
    for (int i = 0; i < diff->frames; i++)
    {
        read_ret = diff->read(diff, (char *)in, frame_size);
        if (read_ret != frame_size)
        {
            if (read_ret == -1)
                ret = RK_AUDIO_FAILURE;
            else
                ret = RK_AUDIO_SUCCESS;
            break;
        }

        /* dsd to pcm */
        int len = dsd2pcm_converter_convert(dsd2pcm, in, frame_size, out, out_size, 0);
        write_ret = diff->write(diff, (char *)out, len * sizeof(uint32_t));
        if (write_ret < 0)
        {
            ret = RK_AUDIO_FAILURE;
            break;
        }
    }
    dsd2pcm_converter_deinit(dsd2pcm);
dsd_err:
    audio_free(in_src);
in_err:
    audio_free(out_src);
out_err:

    return ret;
}

