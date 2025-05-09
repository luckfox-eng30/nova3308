/* bitstream.c */

#include "mp3_enc_types.h"

static void encodeSideInfo(mp3_enc *mp3);
static int encodeMainData(mp3_enc *mp3);
static void Huffmancodebits(mp3_enc *mp3, int *ix, gr_info *gi);
static int HuffmanCode(int table_select, int x, int y, unsigned long *code,
                       unsigned long *extword, int *codebits, int *extbits);
static int L3_huffman_coder_count1(mp3_enc *mp3, struct huffcodetab *h,
                                   int v, int w, int x, int y);

/* bitstream buffers.
 * At a 24K samplerate and 8k bitrate in stereo, sideinfo can be up to 86 frames
 * ahead of its main data ( very silly combination ) */

/*
 * putbytes
 * --------
 * put n bytes in the output buffer. Empty the buffer when full.
 */
static void putbytes(bs_t *bs, unsigned char *data, int n)
{
    unsigned int free = BUFFER_SIZE - bs->i;

    if (n >= free)
    {
        printf("Buffer too small %d >= %d\n", n, free);
    }
    else
    {
        memcpy(&bs->b[bs->i], data, n);
        bs->i += n;
    }
}

/*
 * open_bit_stream
 * ---------------
 * open the device to write the bit stream into it
 */
void open_bit_stream(mp3_enc *mp3)
{
    mp3->bs.b = malloc(BUFFER_SIZE);
    mp3->bs.i = 0;

    /* setup header (the only change between frames is the padding bit) */
    mp3->header[0] = 0xff;

    mp3->header[1] = 0xe0 |
                     (mp3->config.mpeg.type << 3) |
                     (mp3->config.mpeg.layr << 1) |
                     !mp3->config.mpeg.crc;

    mp3->header[2] = (mp3->config.mpeg.bitrate_index << 4) |
                     (mp3->config.mpeg.samplerate_index << 2) |
                     /* mp3->config,mpeg.padding inserted later */
                     mp3->config.mpeg.ext;

    mp3->header[3] = (mp3->config.mpeg.mode << 6) |
                     (mp3->config.mpeg.mode_ext << 4) |
                     (mp3->config.mpeg.copyright << 3) |
                     (mp3->config.mpeg.original << 2) |
                     mp3->config.mpeg.emph;
}

void close_bit_stream(mp3_enc *mp3)
{
    if (mp3 && mp3->bs.b)
        free(mp3->bs.b);
}

/*
 * L3_format_bitstream
 * -------------------
 * This is called after a frame of audio has been quantized and coded.
 * It will write the encoded audio to the bitstream. Note that
 * from a layer3 encoder's perspective the bit stream is primarily
 * a series of main_data() blocks, with header and side information
 * inserted at the proper locations to maintain framing. (See Figure A.7
 * in the IS).
 *
 * note. both header/sideinfo and main data are multiples of 8 bits.
 * this means that the encoded data can be manipulated as bytes
 * which is easier and quicker than bits.
 */

unsigned int L3_format_bitstream(mp3_enc *mp3, unsigned char **ppOutBuf)
{
    int main_bytes;

    encodeSideInfo(mp3);   /* store in fifo */
    main_bytes = encodeMainData(mp3);
    /* send data */
    mp3->by = 0;
    mp3->bs.i = 0;
    while (main_bytes)
    {
        if (!mp3->count)
        {
            /* end of frame so output next header/sideinfo */
            putbytes(&mp3->bs, mp3->fifo[mp3->rd].side, mp3->fifo[mp3->rd].si_len);//13byte 帧头
            mp3->count = mp3->fifo[mp3->rd].fr_len;
            if (++mp3->rd == FIFO_SIZE) mp3->rd = 0; /* point to next header/sideinfo */
        }
        if (main_bytes <= mp3->count)
        {
            /* enough room in frame to output rest of main data, this will exit the while loop */
            putbytes(&mp3->bs, &mp3->main_[mp3->by], main_bytes);
            mp3->count -= main_bytes;
            main_bytes = 0;
        }
        else
        {
            /* fill current frame up, start new frame next time around the while loop */
            putbytes(&mp3->bs, &mp3->main_[mp3->by], mp3->count);
            main_bytes -= mp3->count;
            mp3->by += mp3->count;
            mp3->count = 0;
        }
    }
    *ppOutBuf = mp3->bs.b;

    return mp3->bs.i;
}
/*
 * putbits:
 * --------
 * write N bits into the encoded data buffer.
 * buf = encoded data buffer
 * val = value to write into the buffer
 * n = number of bits of val
 *
 * Bits in value are assumed to be right-justified.
 */
void putbits(mp3_enc *mp3, unsigned char *buf, unsigned long val, unsigned int n)
{
    static int mask[9] = {0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};
    int k, tmp;
    if (n > 32)
        printf("Cannot write more than 32 bits at a time\n");

    mp3->bits += n;

    while (n)
    {
        k = (n < mp3->bi) ? n : mp3->bi;
        tmp = val >> (n - k);
        buf[mp3->by] |= (tmp & mask[k]) << (mp3->bi - k);
        mp3->bi -= k;
        n -= k;
        if (!mp3->bi)
        {
            mp3->bi = 8;
            mp3->by++;
            buf[mp3->by] = 0;
        }
    }

}

/*
 * encodeMainData
 * --------------
 * Encodes the spectrum and places the coded
 * main data in the buffer main.
 * Returns the number of bytes stored.
 */
int encodeMainData(mp3_enc *mp3)
{
    int gr, ch;
    L3_side_info_t  *si = &mp3->side_info;

    mp3->bits = 0;
    mp3->by = 0;
    mp3->bi = 8;
    mp3->main_[0] = 0;

    /* huffmancodes plus reservoir stuffing */
    for (gr = 0; gr < mp3->config.mpeg.granules; gr++)
        for (ch = 0; ch < mp3->config.mpeg.channels; ch++)
            Huffmancodebits(mp3, mp3->l3_enc[gr][ch], &si->gr[gr].ch[ch].tt);   /* encode the spectrum */

    /* ancillary data, used for reservoir stuffing overflow */
    if (si->resv_drain)
    {
        int words = si->resv_drain >> 5;//si->resv_drain上一帧main data后剩余的bit数
        int remainder = si->resv_drain & 31;
        if (si->resv_drain > main_size)
        {
            printf("mp3->main_ too little %d\n", si->resv_drain);
        }
        /* pad with zeros */
        while (words--)
            putbits(mp3, mp3->main_, 0, 32);
        if (remainder)
            putbits(mp3, mp3->main_, 0, remainder);
    }

    return mp3->bits >> 3;
}

/*
 * encodeSideInfo
 * --------------
 * Encodes the header and sideinfo and stores the coded data
 * in the side fifo for transmission at the appropriate place
 * in the bitstream.
 */
void encodeSideInfo(mp3_enc *mp3)
{
    L3_side_info_t  *si = &mp3->side_info;
    int gr, ch, region;
    unsigned char *sf = mp3->fifo[mp3->wr].side;

    /* header */
    *sf     = mp3->header[0];
    *(sf + 1) = mp3->header[1];
    *(sf + 2) = mp3->header[2] | (mp3->config.mpeg.padding << 1);
    *(sf + 3) = mp3->header[3];
    *(sf + 4) = 0;

    mp3->bits = 32; //前4个char的bit数，putbits中会累加
    mp3->by = 4;   //前4个char的字节数 ，by在putbits也会累加
    mp3->bi = 8; //

    /* side info */
    if (mp3->config.mpeg.type == MPEG1)
    {
        putbits(mp3, sf, si->main_data_begin, 9);
        putbits(mp3, sf, 0, (mp3->config.mpeg.channels == 2) ? 3 : 5);    /* private bits */

        for (ch = 0; ch < mp3->config.mpeg.channels; ch++)
            putbits(mp3, sf, 0, 4);  /* scfsi */

        for (gr = 0; gr < 2; gr++)
            for (ch = 0; ch < mp3->config.mpeg.channels ; ch++)
            {
                gr_info *gi = &(si->gr[gr].ch[ch].tt);
                putbits(mp3, sf, gi->part2_3_length,    12);
                putbits(mp3, sf, gi->big_values,         9);
                putbits(mp3, sf, gi->global_gain,        8);
                putbits(mp3, sf, 0, 5);  /* scalefac_compress, window switching flag */

                for (region = 0; region < 3; region++)
                    putbits(mp3, sf, gi->table_select[region], 5);

                putbits(mp3, sf, gi->region0_count,      4);
                putbits(mp3, sf, gi->region1_count,      3);
                putbits(mp3, sf, 0, 2);  /* preflag, scalefac_scale */
                putbits(mp3, sf, gi->count1table_select, 1);
            }
    }
    else /* mpeg 2/2.5 */
    {
        putbits(mp3, sf, si->main_data_begin, 8);
        putbits(mp3, sf, 0, (mp3->config.mpeg.channels == 1) ? 1 : 2);    /* private bits */

        for (ch = 0; ch < mp3->config.mpeg.channels ; ch++)
        {
            gr_info *gi = &(si->gr[0].ch[ch].tt);
            putbits(mp3, sf, gi->part2_3_length,    12);
            putbits(mp3, sf, gi->big_values,         9);
            putbits(mp3, sf, gi->global_gain,        8);
            putbits(mp3, sf, 0, 10);  /* scalefac_compress, window switching flag */

            for (region = 0; region < 3; region++)
                putbits(mp3, sf, gi->table_select[region], 5);

            putbits(mp3, sf, gi->region0_count,      4);
            putbits(mp3, sf, gi->region1_count,      3);
            putbits(mp3, sf, 0, 1);  /* scalefac_scale */
            putbits(mp3, sf, gi->count1table_select, 1);
        }
    }
    mp3->fifo[mp3->wr].fr_len = (mp3->config.mpeg.bits_per_frame - mp3->bits) >> 3; /* data bytes in this frame */
    mp3->fifo[mp3->wr].si_len = mp3->bits >> 3;
    /* bytes in side info */
    if (++mp3->wr == FIFO_SIZE) mp3->wr = 0; /* point to next buffer */

}

/*
 * Huffmancodebits
 * ---------------
 * Note the discussion of huffmancodebits() on pages 28
 * and 29 of the IS, as well as the definitions of the side
 * information on pages 26 and 27.
 */
void Huffmancodebits(mp3_enc *mp3, int *ix, gr_info *gi)
{
    int region1Start;
    int region2Start;
    int i, bigvalues, count1End;
    int v, w, x, y, cx_bits, cbits, xbits, stuffingBits;
    unsigned long code, ext;
    struct huffcodetab *h;
    int tablezeros, r0, r1, r2, rt, *pr;
//  int bvbits, c1bits;
    int bitsWritten = 0;
//  int idx = 0;
    tablezeros = 0;
    r0 = r1 = r2 = 0;

    /* 1: Write the bigvalues */
    bigvalues = gi->big_values << 1;
    {
        unsigned scalefac_index = 100;

        scalefac_index = gi->region0_count + 1;
        region1Start = mp3->scalefac_band_long[ scalefac_index ];
        scalefac_index += gi->region1_count + 1;
        region2Start = mp3->scalefac_band_long[ scalefac_index ];

        for (i = 0; i < bigvalues; i += 2)
        {
            unsigned tableindex = 100;

            /* get table pointer */
            if (i < region1Start)
            {
                tableindex = gi->table_select[0];
                pr = &r0;
            }
            else if (i < region2Start)
            {
                tableindex = gi->table_select[1];
                pr = &r1;
            }
            else
            {
                tableindex = gi->table_select[2];
                pr = &r2;
            }

            h = &ht[ tableindex ];
            /* get huffman code */
            x = ix[i];
            y = ix[i + 1];
            if (tableindex)
            {
                cx_bits = HuffmanCode(tableindex, x, y, &code, &ext, &cbits, &xbits);
                putbits(mp3, mp3->main_,  code, cbits);
                putbits(mp3, mp3->main_,  ext, xbits);
                bitsWritten += rt = cx_bits;
                *pr += rt;
            }
            else
            {
                tablezeros += 1;
                *pr = 0;
            }
        }
    }
    //bvbits = bitsWritten;

    /* 2: Write count1 area */
    h = &ht[gi->count1table_select + 32];
    count1End = bigvalues + (gi->count1 << 2);
    for (i = bigvalues; i < count1End; i += 4)
    {
        v = ix[i];
        w = ix[i + 1];
        x = ix[i + 2];
        y = ix[i + 3];
        bitsWritten += L3_huffman_coder_count1(mp3, h, v, w, x, y);
    }
    //c1bits = bitsWritten - bvbits;
    if ((stuffingBits = gi->part2_3_length - bitsWritten) != 0)
    {
        int words = stuffingBits >> 5;
        int remainder = stuffingBits & 31;

        /*
         * Due to the nature of the Huffman code
         * tables, we will pad with ones
         */
        while (words--)
            putbits(mp3, mp3->main_, ~0, 32);
        if (remainder)
            putbits(mp3, mp3->main_, ~0, remainder);
    }
}

/*
 * abs_and_sign
 * ------------
 */
int abs_and_sign(int *x)
{
    if (*x > 0) return 0;
    *x *= -1;
    return 1;
}

/*
 * L3_huffman_coder_count1
 * -----------------------
 */
int L3_huffman_coder_count1(mp3_enc *mp3, struct huffcodetab *h, int v, int w, int x, int y)
{
    HUFFBITS huffbits;
    unsigned int signv, signw, signx, signy, p;
    int len;
    int totalBits;

    signv = abs_and_sign(&v);
    signw = abs_and_sign(&w);
    signx = abs_and_sign(&x);
    signy = abs_and_sign(&y);

    p = v + (w << 1) + (x << 2) + (y << 3);
    huffbits = h->table[p];
    len = h->hlen[p];
    putbits(mp3, mp3->main_,  huffbits, len);
    totalBits = len;
    if (v)
    {
        putbits(mp3, mp3->main_,  signv, 1);
        totalBits++;
    }
    if (w)
    {
        putbits(mp3, mp3->main_,  signw, 1);
        totalBits++;
    }

    if (x)
    {
        putbits(mp3, mp3->main_,  signx, 1);
        totalBits++;
    }
    if (y)
    {
        putbits(mp3, mp3->main_,  signy, 1);
        totalBits++;
    }
    return totalBits;
}

/*
 * HuffmanCode
 * -----------
 * Implements the pseudocode of page 98 of the IS
 */
int HuffmanCode(int table_select, int x, int y, unsigned long *code,
                unsigned long *ext, int *cbits, int *xbits)
{
    unsigned signx, signy, linbitsx, linbitsy, linbits, ylen, idx;
//  unsigned xlen;
    struct huffcodetab *h;

    *cbits = 0;
    *xbits = 0;
    *code  = 0;
    *ext   = 0;

    if (table_select == 0) return 0;

    signx = abs_and_sign(&x);
    signy = abs_and_sign(&y);
    h = &(ht[table_select]);
//  xlen = h->xlen;
    ylen = h->ylen;
    linbits = h->linbits;
    linbitsx = linbitsy = 0;

    if (table_select > 15)
    {
        /* ESC-table is used */
        if (x > 14)
        {
            linbitsx = x - 15;
            x = 15;
        }
        if (y > 14)
        {
            linbitsy = y - 15;
            y = 15;
        }

        idx = (x * ylen) + y;
        *code  = h->table[idx];
        *cbits = h->hlen [idx];
        if (x > 14)
        {
            *ext   |= linbitsx;
            *xbits += linbits;
        }
        if (x != 0)
        {
            *ext = ((*ext) << 1) | signx;
            *xbits += 1;
        }
        if (y > 14)
        {
            *ext = ((*ext) << linbits) | linbitsy;
            *xbits += linbits;
        }
        if (y != 0)
        {
            *ext = ((*ext) << 1) | signy;
            *xbits += 1;
        }
    }
    else
    {
        /* No ESC-words */
        idx = (x * ylen) + y;
        *code = h->table[idx];
        *cbits += h->hlen[ idx ];
        if (x != 0)
        {
            *code = ((*code) << 1) | signx;
            *cbits += 1;
        }
        if (y != 0)
        {
            *code = ((*code) << 1) | signy;
            *cbits += 1;
        }
    }
    return *cbits + *xbits;
}


//#pragma arm section code
//#endif





