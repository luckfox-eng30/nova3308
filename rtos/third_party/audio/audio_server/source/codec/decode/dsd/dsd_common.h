#ifndef __DSD_COMMON_H__
#define __DSD_COMMON_H__

typedef enum {
    DSD64 = 0,
    DSD128,
    DSD256
} dec_type;

struct dsd_buf
{
    uint8_t *buf;
    uint32_t size;
    uint32_t alloc_size;
};

struct chunk
{
    uint8_t id[4];
    uint8_t size[8];
};

int chunk_id_is(char *chunk, char *id);
uint32_t data_le32(uint8_t *data);
uint16_t data_le16(uint8_t *data);
uint32_t data_be64(uint8_t *data);
uint32_t data_be32(uint8_t *data);
uint16_t data_be16(uint8_t *data);
uint8_t *resize_dsd_buf(struct dsd_buf *buf, uint32_t new_size);

#endif

