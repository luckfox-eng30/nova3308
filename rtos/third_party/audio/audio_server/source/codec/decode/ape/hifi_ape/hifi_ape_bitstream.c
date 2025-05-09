#include "AudioConfig.h"
#include "ape.h"

#ifdef A_CORE_DECODE
#ifdef HIFI_APE_DECODE

u8 get_ape_bits(ByteIOContext *pb, u8 len)
{
    u8 res_data, new_data;

    res_data = 0;
    pb->size_in_bits -= len;

    if (pb->bit_left < len)
    {
        res_data = pb->buffer[pb->bye_index];
        res_data &= ((1 << pb->bit_left) - 1);
        len -= pb->bit_left;
        pb->bye_index++;
        pb->bit_left = 8;
        res_data = (res_data << len);

        new_data = pb->buffer[pb->bye_index];
        new_data &= ((1 << pb->bit_left) - 1);
        pb->bit_left -= len;
        new_data = (new_data >> pb->bit_left);
        res_data += new_data;
    }
    else
    {
        res_data = pb->buffer[pb->bye_index];
        res_data &= ((1 << pb->bit_left) - 1);
        pb->bit_left -= len;
        res_data = (res_data >> pb->bit_left);
    }

    return res_data;
}

void fill_buf(ByteIOContext *pb, int bytes)
{
    ape_fread(pb->buffer, 1, bytes, pb->userdata);
    pb->bye_index = 0;
    pb->bit_left = 8;
    pb->size_in_bits = bytes * 8;
}

u8 get_bitbye(ByteIOContext *pb, u8 len)
{
    u8 res_data;

    res_data = 0;
    if (pb->size_in_bits < len)
    {
        res_data = get_ape_bits(pb, pb->size_in_bits);
        len -= pb->size_in_bits;
        fill_buf(pb, len / 8);
        res_data = (res_data << len);
        res_data += get_ape_bits(pb, len);
    }
    else
    {
        res_data = get_ape_bits(pb, len);
    }

    return res_data;
}

u16 get_bitshort(ByteIOContext *pb, u8 len)
{
    u16 res_data;

    res_data = 0;
    while (len >= 8)
    {
        res_data = (res_data << 8);
        res_data += get_bitbye(pb, 8);
        len -= 8;
    }
    res_data = (res_data << len);
    res_data += get_bitbye(pb, len);

    return res_data;
}

u32 get_bitlong(ByteIOContext *pb, u8 len)
{
    u32 res_data;

    res_data = 0;
    while (len >= 8)
    {
        res_data = (res_data << 8);
        res_data += get_bitbye(pb, 8);
        len -= 8;
    }
    res_data = (res_data << len);
    res_data += get_bitbye(pb, len);

    return res_data;
}

uint32_t get_le16(ByteIOContext *pb)
{
    u16 res_data, b_s_data;

    res_data = get_bitshort(pb, 16);

    b_s_data = ((res_data >> 8) & 0xff) | (((res_data) & 0xff) << 8);

    res_data = b_s_data;
    return res_data;
}

uint32_t get_le32(ByteIOContext *pb)
{
    u32 res_data, b_s_data;

    res_data = get_bitlong(pb, 32);

    b_s_data = ((res_data >> 24) & 0xff) | (((res_data >> 16) & 0xff) << 8) | (((res_data >> 8) & 0xff) << 16) | (((res_data) & 0xff) << 24);

    res_data = b_s_data;
    return res_data;
}

void url_fskip(ByteIOContext *pb, u16 len)
{
    while (len)
    {
        get_bitbye(pb, 8);
        len -= 1;
    }
}

void url_fseek(ByteIOContext *pb, u16 len, u8 type)
{
    if (type == SEEK_CUR)
    {
        while (len)
        {
            get_bitbye(pb, 8);
            len -= 1;
        }
    }
}

void get_buffer(ByteIOContext *pb, u8 *buf, u8 len)
{
    u8 *ptr = buf;
    while (len--)
    {
        *ptr++ = get_bitbye(pb, 8);
    }
}

void freebuf(ByteIOContext *pb)
{
    pb->size_in_bits = 0;
    pb->size_in_bits = 0;
}

u32 fread32(ByteCache *datas)
{
    u32 res_data;
    if (datas->cacheindex == 0)
    {
        ape_fread((uint8_t *)&datas->cachedata, 1, 4, datas->userdata);
        res_data = datas->cachedata;
    }
    else
    {
        res_data = (datas->cachedata << (32 - datas->cacheindex));
        ape_fread((uint8_t *)&datas->cachedata, 1, 4, datas->userdata);
        res_data += ((datas->cachedata >> datas->cacheindex) & ((1 << (32 - datas->cacheindex)) - 1));
    }
    return res_data;
}

u16 fread16(ByteCache *datas)
{
    u16 res_data;

    if (datas->cacheindex == 0)
    {
        ape_fread((uint8_t *)&datas->cachedata, 1, 4, datas->userdata);
        res_data = ((datas->cachedata >> 16) & 0xffff);
        datas->cacheindex = 16;
    }
    else if (datas->cacheindex == 8)
    {
        res_data = ((datas->cachedata & 0xff) << 8);
        datas->cacheindex = 24;
        ape_fread((uint8_t *)&datas->cachedata, 1, 4, datas->userdata);
        res_data += ((datas->cachedata >> 24) & 0xff);
    }
    else
    {
        datas->cacheindex -= 16;
        res_data = ((datas->cachedata >> datas->cacheindex) & 0xffff);
    }
    return res_data;
}

u8 fread8(ByteCache *datas)
{
    u8 res_data;

    if (datas->cacheindex == 0)
    {
        ape_fread((uint8_t *)&datas->cachedata, 1, 4, datas->userdata);
        res_data = ((datas->cachedata >> 24) & 0xff);
        datas->cacheindex = 24;
    }
    else
    {
        datas->cacheindex -= 8;
        res_data = ((datas->cachedata >> datas->cacheindex) & 0xff);
    }
    return res_data;
}

#define malloc_size UINT_MAX
//const char malloc_buff[malloc_size];
static char *malloc_buff = NULL;
static long malloc_buff_pos = 0;
void av_free()
{
    malloc_buff_pos = 0;
    if (malloc_buff)
        audio_free(malloc_buff);
    malloc_buff = NULL;
}
void *av_malloc(int n)
{
    if (malloc_buff == NULL)
    {
        malloc_buff = audio_malloc(malloc_size);
        if (!malloc_buff)
            return NULL;
    }
    malloc_buff_pos += n;
    if (malloc_buff_pos >= malloc_size)
    {
        RK_AUDIO_LOG_E(" malloc buf error %ld %d\n", malloc_buff_pos, n);
    }
    return (void *)&malloc_buff[malloc_buff_pos - n];
}

u32 Blockout(int32_t *decoded0, int32_t *decoded1, u8 *outbuf, u16 len, int ch, u32 bps)
{
    int i;
    u8 *samples = (u8 *)outbuf;
#if !DISABLE_24BIT_OUT
    if (bps == 24)
    {
        for (i = 0; i < len; i++)
        {
            samples[0] = decoded0[i] & 0xFF;
            samples[1] = (decoded0[i] >> 8) & 0xFF;
            samples[2] = (decoded0[i] >> 16) & 0xFF;
            samples += 3;
            if (ch == 2)
            {
                samples[0] = decoded1[i] & 0xFF;
                samples[1] = (decoded1[i] >> 8) & 0xFF;
                samples[2] = (decoded1[i] >> 16) & 0xFF;
                samples += 3;
            }
            else
            {
                samples[0] = decoded0[i] & 0xFF;
                samples[1] = (decoded0[i] >> 8) & 0xFF;
                samples[2] = (decoded0[i] >> 16) & 0xFF;
                samples += 3;
            }
        }

        return len * 2 * 3;
    }
    else
#endif
    {
        for (i = 0; i < len; i++)
        {
            *(int16_t *)samples = (int16_t)decoded0[i];
            samples += 2;
            if (ch == 2)
            {
                *(int16_t *)samples = (int16_t)decoded1[i];
                samples += 2;
            }
            else
            {
                *(int16_t *)samples = (int16_t)decoded0[i];
                samples += 2;
            }
        }

        return len * 2 * 2;
    }
}
#endif
#endif
