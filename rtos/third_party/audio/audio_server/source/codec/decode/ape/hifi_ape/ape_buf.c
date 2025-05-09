#include <stdio.h>
#include <string.h>
#include "ape.h"

#if !DATA_FROM_FILE
int ape_fread(uint8_t *buf, int size, int count, void *fd)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;

    if (in->buf.buf_in_left < size * count)
    {
        if (in->cb(in->cb_param) <= 0)
            return 0;
    }

    memcpy(buf, in->buf.buf_in + (in->buf.buf_in_size - in->buf.buf_in_left), count * size);

    in->read_bytes += size * count;
    in->buf.file_tell += size * count;
    in->buf.buf_in_left -= size * count;

    return size * count;
}

int ape_fseek(void *fd, int offset, int mode)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;

    if ((in->buf.buf_in_left - offset) < 0)
    {
        printf("warning: %d %ld %ld\n", offset, in->buf.buf_in_left, in->buf.buf_in_size);
        offset = in->buf.buf_in_left;
    }
    if ((in->buf.buf_in_left - offset) > in->buf.buf_in_size)
    {
        printf("warning: %d %ld %ld\n", offset, in->buf.buf_in_left, in->buf.buf_in_size);
        offset = in->buf.buf_in_left - in->buf.buf_in_size;
    }
    in->buf.buf_in_left -= offset;
    in->buf.file_tell += offset;

    return 0;
}

int ape_ftell(void *fd)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;
    return in->buf.file_tell;
}

long ape_fsize(void *fd)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;

    return in->buf.file_size;
}
#endif
