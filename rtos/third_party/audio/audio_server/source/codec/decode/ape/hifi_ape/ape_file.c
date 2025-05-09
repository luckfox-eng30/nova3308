#include <stdio.h>
#include "ape.h"

#if DATA_FROM_FILE
int ape_fread(uint8_t *buf, int size, int count, void *fd)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;
    in->read_bytes += size * count;
    fread(buf, size, count, in->fd);
}

int ape_fseek(void *fd, int offset, int mode)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;
    printf("%s %d %d\n", __func__, offset, ftell(in->fd));
    fseek(in->fd, offset, mode);
}

int ape_ftell(void *fd)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;
    printf("%s %d\n", __func__, ftell(in->fd));
    return ftell(in->fd);
}

long ape_fsize(void *fd)
{
    struct APEDecIn *in = (struct APEDecIn *)fd;
    long cur_pos = ftell(in->fd);
    long len;

    fseek(in->fd, 0, SEEK_END);
    len = ftell(in->fd);
    fseek(in->fd, cur_pos, SEEK_SET);

    return len;
}
#endif
