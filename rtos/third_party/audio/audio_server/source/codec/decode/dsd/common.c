#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "AudioConfig.h"
#include "dsd_common.h"

int chunk_id_is(char *chunk, char *id)
{
    return ((chunk[0] == id[0]) &&
            (chunk[1] == id[1]) &&
            (chunk[2] == id[2]) &&
            (chunk[3] == id[3]));
}

uint32_t data_le32(uint8_t *data)
{
    return ((data[3] << 24) |
            (data[2] << 16) |
            (data[1] << 8) |
            (data[0] << 0));
}

uint16_t data_le16(uint8_t *data)
{
    return ((data[1] << 8) |
            (data[0] << 0));
}

uint32_t data_be64(uint8_t *data)
{
    return ((data[4] << 24) |
            (data[5] << 16) |
            (data[6] << 8) |
            (data[7] << 0));
}

uint32_t data_be32(uint8_t *data)
{
    return ((data[0] << 24) |
            (data[1] << 16) |
            (data[2] << 8) |
            (data[3] << 0));
}

uint16_t data_be16(uint8_t *data)
{
    return ((data[0] << 8) |
            (data[1] << 0));
}

uint8_t *resize_dsd_buf(struct dsd_buf *buf, uint32_t new_size)
{
    if (new_size == 0)
    {
        if (buf->buf)
            audio_free(buf->buf);
        buf->buf = NULL;
        return NULL;
    }

    if (buf->buf == NULL)
    {
        buf->alloc_size = new_size;
        buf->buf = audio_malloc(new_size);
    }
    else if (buf->alloc_size < new_size)
    {
        char *tmp;
        tmp = audio_realloc(buf->buf, new_size);
        if (!tmp)
        {
            audio_free(buf->buf);
            return NULL;
        }
        buf->alloc_size = new_size;
    }
    buf->size = new_size;

    return buf->buf;
}

