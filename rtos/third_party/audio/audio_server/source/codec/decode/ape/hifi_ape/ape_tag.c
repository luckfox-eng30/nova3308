/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-12     Jair Wu      First version
 *
 */

#include "play_ape.h"
#include "ape.h"

static inline uint16_t _get_le16(char *buf)
{
    return ((buf[1] << 8) + buf[0]);
}

static inline uint32_t _get_le32(char *buf)
{
    return ((buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0]);
}

static int ape_in = 0;
int ape_input(APEDec *dec, struct play_ape *ape, char *buf, int len)
{
    int ret;

    ret = ape->input(ape->userdata, buf, len);
    if (ret > 0)
        dec->in.buf.file_tell += ret;

    ape_in += ret;

    return ret;
}

int ape_read_header(APEDec *dec, struct play_ape *ape)
{
#define TEMP_BUF_SIZE 1024
    APEContext *apec = dec->apeobj;
    char tag[4];
    int i;

    apec->junklength = 0;

    if (ape_input(dec, ape, (char *)tag, 4) != 4)
        return -3;
    if (memcmp(tag, "MAC ", 4))
        return -1;
    // uint32_t tag = _get_le32(pb);
    // if (tag != MKTAG('M', 'A', 'C', ' '))
    //     return -1;
    if (ape_input(dec, ape, (char *)&apec->fileversion, 2) != 2)
        return -3;
    // ape->fileversion = _get_le16(pb);

    if (apec->fileversion < APE_MIN_VERSION || apec->fileversion > APE_MAX_VERSION)
    {
        return -2;
    }

    char *buf = audio_malloc(TEMP_BUF_SIZE);
    if (!buf)
        return -4;
    if (apec->fileversion >= 3980)
    {
        char *_buf = buf;
        if (ape_input(dec, ape, _buf, 46) != 46)
        {
            audio_free(buf);
            return -3;
        }
        apec->padding1             = _get_le16(_buf);
        _buf += 2;
        apec->descriptorlength     = _get_le32(_buf);
        _buf += 4;
        apec->headerlength         = _get_le32(_buf);
        _buf += 4;
        apec->seektablelength      = _get_le32(_buf);
        _buf += 4;
        apec->wavheaderlength      = _get_le32(_buf);
        _buf += 4;
        apec->audiodatalength      = _get_le32(_buf);
        _buf += 4;
        apec->audiodatalength_high = _get_le32(_buf);
        _buf += 4;
        apec->wavtaillength        = _get_le32(_buf);
        _buf += 4;
        memcpy(apec->md5, _buf, 16);
        // get_buffer(pb, apec->md5, 16);

        /* Skip any unknown bytes at the end of the descriptor.
           This is for future compatibility */
        if (apec->descriptorlength > 52)
        {
            // url_fseek(pb, apec->descriptorlength - 52, SEEK_CUR);
            uint32_t len = apec->descriptorlength - 52;
            uint32_t _len;
            while (len)
            {
                if (len > TEMP_BUF_SIZE)
                    _len = TEMP_BUF_SIZE;
                else
                    _len = len;
                if (ape_input(dec, ape, buf, _len) != _len)
                {
                    audio_free(buf);
                    return -3;
                }
                len -= _len;
            }
        }

        if (ape_input(dec, ape, buf, 24) != 24)
        {
            audio_free(buf);
            return -3;
        }
        _buf = buf;
        /* Read header data */
        apec->compressiontype      = _get_le16(_buf);
        _buf += 2;
        apec->formatflags          = _get_le16(_buf);
        _buf += 2;
        apec->blocksperframe       = _get_le32(_buf);
        _buf += 4;
        apec->finalframeblocks     = _get_le32(_buf);
        _buf += 4;
        apec->totalframes          = _get_le32(_buf);
        _buf += 4;
        apec->bps                  = _get_le16(_buf);
        _buf += 2;
        apec->channels             = _get_le16(_buf);
        _buf += 2;
        apec->samplerate           = _get_le32(_buf);
    }
    else
    {
        char *_buf = buf;
        if (ape_input(dec, ape, _buf, 26) != 26)
        {
            audio_free(buf);
            return -3;
        }

        apec->descriptorlength = 0;
        apec->headerlength = 32;

        apec->compressiontype      = _get_le16(_buf);
        _buf += 2;
        apec->formatflags          = _get_le16(_buf);
        _buf += 2;
        apec->channels             = _get_le16(_buf);
        _buf += 2;
        apec->samplerate           = _get_le32(_buf);
        _buf += 4;
        apec->wavheaderlength      = _get_le32(_buf);
        _buf += 4;
        apec->wavtaillength        = _get_le32(_buf);
        _buf += 4;
        apec->totalframes          = _get_le32(_buf);
        _buf += 4;
        apec->finalframeblocks     = _get_le32(_buf);
        _buf += 4;

        if (apec->formatflags & MAC_FORMAT_FLAG_HAS_PEAK_LEVEL)
        {
            // url_fseek(pb, 4, SEEK_CUR); /* Skip the peak level */
            if (ape_input(dec, ape, _buf, 4) != 4)
            {
                audio_free(buf);
                return -3;
            }
            apec->headerlength += 4;
        }

        if (apec->formatflags & MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS)
        {
            if (ape_input(dec, ape, buf, 4) != 4)
            {
                audio_free(buf);
                return -3;
            }
            apec->seektablelength = _get_le32(buf);
            apec->headerlength += 4;
            apec->seektablelength *= sizeof(int32_t);
        }
        else
            apec->seektablelength = apec->totalframes * sizeof(int32_t);

        if (apec->formatflags & MAC_FORMAT_FLAG_8_BIT)
            apec->bps = 8;
        else if (apec->formatflags & MAC_FORMAT_FLAG_24_BIT)
            apec->bps = 24;
        else
            apec->bps = 16;

        if (apec->fileversion >= 3950)
            apec->blocksperframe = 73728 * 4;
        else if (apec->fileversion >= 3900 || (apec->fileversion >= 3800  && apec->compressiontype >= 4000))
            apec->blocksperframe = 73728;
        else
            apec->blocksperframe = 9216;

        /* Skip any stored wav header */
        if (!(apec->formatflags & MAC_FORMAT_FLAG_CREATE_WAV_HEADER))
        {
            // url_fskip(pb, apec->wavheaderlength);
            uint32_t len = apec->wavheaderlength;
            uint32_t _len;
            while (len)
            {
                if (len > TEMP_BUF_SIZE)
                    _len = TEMP_BUF_SIZE;
                else
                    _len = len;
                if (ape_input(dec, ape, buf, _len) != _len)
                {
                    audio_free(buf);
                    return -3;
                }
                len -= _len;
            }
        }
    }
    // apec->file_size = apec_fsize(&dec->in);
    apec->total_blocks = apec->blocksperframe * (apec->totalframes - 1) + apec->finalframeblocks;
    apec->file_time = (uint32_t)(((double)(apec->total_blocks) * (double)(1000)) / (double)(apec->samplerate));
    apec->bitrate = (uint32_t)(((long long)(apec->file_size - dec->ID3_len) * 8000) / (double)(apec->file_time));
    RK_AUDIO_LOG_V("APE version %d "
                   "compression %d",
                   apec->fileversion, apec->compressiontype);
//    RK_AUDIO_LOG_V("totalframes %ld "
//                   "blocksperframe %ld "
//                   "file_size %ld "
//                   "file_time %ld",
//                   apec->totalframes, apec->blocksperframe, apec->file_size, apec->file_time);
    RK_AUDIO_LOG_V("samplerate %ld "
                   "bps %d "
                   "channels %d "
                   "bitrate %ld",
                   apec->samplerate, apec->bps, apec->channels, apec->bitrate);

    if (apec->totalframes > (UINT_MAX / 36))
    {
        RK_AUDIO_LOG_V("totalframes %ld >1800 ", apec->totalframes);
        return -3;
    }

    apec->firstframe   = apec->junklength + apec->descriptorlength + apec->headerlength + apec->seektablelength + apec->wavheaderlength;
    apec->currentframe = 0;

    apec->totalsamples = apec->finalframeblocks;
    if (apec->totalframes > 1)
        apec->totalsamples += apec->blocksperframe * (apec->totalframes - 1);

    if (apec->seektablelength > 0)
    {
        apec->seektable = (uint32_t *)audio_malloc(apec->totalframes * 4);
        if (ape_input(dec, ape, buf, apec->totalframes * 4) != apec->totalframes * 4)
        {
            audio_free(buf);
            return -3;
        }
        char *_buf = buf;
        for (i = 0; i < apec->totalframes; i++)
        {
            apec->seektable[i] = _get_le32(_buf) + dec->ID3_len;
            _buf += 4;
        }

        // url_fskip(pb, (apec->seektablelength - (apec->totalframes * 4)));
        uint32_t len = apec->seektablelength - (apec->totalframes * 4);
        uint32_t _len;
        while (len)
        {
            if (len > TEMP_BUF_SIZE)
                _len = TEMP_BUF_SIZE;
            else
                _len = len;
            if (ape_input(dec, ape, buf, _len) != _len)
            {
                audio_free(buf);
                return -3;
            }
            len -= _len;
        }
    }
    audio_free(buf);

    // apec->frames = (APEFrame *)av_malloc(apec->totalframes * sizeof(APEFrame));

    // if (!apec->frames)
    //     return -4;
    // apec->frames[0].pos     = apec->firstframe + dec->ID3_len;
    // apec->frames[0].nblocks = apec->blocksperframe;
    // apec->frames[0].skip    = 0;
    // for (i = 1; i < apec->totalframes; i++)
    // {
    //     apec->frames[i].pos      = apec->seektable[i]; //apec->frames[i-1].pos + apec->blocksperframe;
    //     apec->frames[i].nblocks  = apec->blocksperframe;
    //     apec->frames[i - 1].size = apec->frames[i].pos - apec->frames[i - 1].pos;
    //     apec->frames[i].skip     = (apec->frames[i].pos - apec->frames[0].pos) & 3;
    // }

    // apec->frames[apec->totalframes - 1].size    = apec->finalframeblocks * 4;
    // apec->frames[apec->totalframes - 1].nblocks = apec->finalframeblocks;

    // for (i = 0; i < apec->totalframes; i++)
    // {
    //     if (apec->frames[i].skip)
    //     {
    //         apec->frames[i].pos  -= apec->frames[i].skip;
    //         apec->frames[i].size += apec->frames[i].skip;
    //     }
    //     apec->frames[i].size = (apec->frames[i].size + 3) & ~3;
    // }

    printf("ape in %d\n", ape_in);

    return 1;
}

int ape_release_header(APEDec *dec)
{
    APEContext *apec = dec->apeobj;

    if (apec->seektable)
        audio_free(apec->seektable);

    return 0;
}
