#ifndef __APE_H__
#define __APE_H__

#include <stdio.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define int64_t_C(c)     (int64_t)(c)
#define uint64_t_C(c)    (uint64_t)(c)

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))

#define FFALIGN(x, a)     (((x) + (a) - 1) & ~((a) - 1))

#define INT_MAX       2147483647    /* maximum (signed) int value */

#define av_log(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9)
#define UINT_MAX  (1800*36)

#include "hifi_ape_bitstream.h"

#define BLOCKS_PER_LOOP     2048
//#define BLOCKS_PER_LOOP     4096

#define MAX_CHANNELS        2
#define MAX_BYTESPERSAMPLE  3
#define APE_FRAMECODE_MONO_SILENCE    1
#define APE_FRAMECODE_STEREO_SILENCE  3
#define APE_FRAMECODE_PSEUDO_STEREO   4
#define HISTORY_SIZE 512
#define PREDICTOR_SIZE 50
#define APE_FILTER_LEVELS 3

#define ENABLE_DEBUG 0
#define DATA_FROM_FILE 0

/* The earliest and latest file formats supported by this library */
#define APE_MIN_VERSION 3950
#define APE_MAX_VERSION 3990

#define MAC_FORMAT_FLAG_8_BIT                 1 // is 8-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_CRC                   2 // uses the new CRC32 error detection [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_PEAK_LEVEL        4 // uint32 nPeakLevel after the header [OBSOLETE]
#define MAC_FORMAT_FLAG_24_BIT                8 // is 24-bit [OBSOLETE]
#define MAC_FORMAT_FLAG_HAS_SEEK_ELEMENTS    16 // has the number of seek elements after the peak level
#define MAC_FORMAT_FLAG_CREATE_WAV_HEADER    32 // create the wave header on decompression (not stored)

#define MAC_SUBFRAME_SIZE 4608

#define APE_EXTRADATA_SIZE 6

typedef struct
{
    int64_t pos;
    int nblocks;
    int size;
    int skip;
    int64_t pts;
} APEFrame;

typedef struct
{
    /* Derived fields */
    uint32_t junklength;
    uint32_t firstframe;
    uint32_t totalsamples;
    int currentframe;
    APEFrame *frames;
    uint32_t APE_Frm_NUM;
    uint32_t frm_left_sample;
    /* Info from Descriptor Block */
    char magic[4];
    int16_t fileversion;
    int16_t padding1;
    uint32_t descriptorlength;
    uint32_t headerlength;
    uint32_t seektablelength;
    uint32_t wavheaderlength;
    uint32_t audiodatalength;
    uint32_t audiodatalength_high;
    uint32_t wavtaillength;
    uint8_t md5[16];

    /* Info from Header Block */
    uint16_t compressiontype;
    uint16_t formatflags;
    uint32_t blocksperframe;
    uint32_t finalframeblocks;
    uint32_t totalframes;
    uint16_t bps;
    uint16_t channels;
    uint32_t samplerate;

    uint32_t file_size;//�ļ��ܴ�С
    uint32_t total_blocks;//�����ݸ���
    uint32_t file_time ;//�ļ�ʱ��
    uint32_t bitrate;
    uint32_t TimePos;
    /* Seektable */
    uint32_t *seektable;
} APEContext;

typedef struct APEFilter
{
    int16_t *coeffs;        ///< actual coefficients used in filtering
    int16_t *adaptcoeffs;   ///< adaptive filter coefficients used for correcting of actual filter coefficients
    int16_t *historybuffer; ///< filter memory
    int16_t *delay;         ///< filtered values
    int avg;
} APEFilter;
typedef struct APERice
{
    uint32_t k;
    uint32_t ksum;
} APERice;
typedef struct APERangecoder
{
    uint32_t low;           ///< low end of interval 48
    uint32_t range;         ///< length of interval  52
    uint32_t help;          ///< bytes_to_follow resp. intermediate value 56
    unsigned int buffer;    ///< buffer for input/output 60
} APERangecoder;
typedef struct APEPredictor
{
    int32_t *buf;
    int32_t lastA[2];
    int32_t filterA[2];
    int32_t filterB[2];
    int32_t coeffsA[2][4];  ///< adaption coefficients
    int32_t coeffsB[2][5];  ///< adaption coefficients
    int32_t historybuffer[HISTORY_SIZE + PREDICTOR_SIZE];
} APEPredictor;
typedef struct APEContextdec
{
    int channels;
    int samples;                             ///< samples left to decode in current frame 0
    int fileversion;                         ///< codec version, very important in decoding process 4
    int compression_level;                   ///< compression levels 8
    int fset;                                ///< which filter set to use (calculated from compression level) 12
    int flags;                               ///< global decoder flags 16
    uint32_t CRC;                            ///< frame CRC 20
    int frameflags;                          ///< frame flags 24
    int currentframeblocks;                  ///< samples (per channel) in current frame 28
    int blocksdecoded;                       ///< count of decoded samples in current frame 32
    APEPredictor predictor;                  ///< predictor used for final reconstruction  2380
    int32_t decoded0[BLOCKS_PER_LOOP];       ///< decoded data for the first channel
    int32_t decoded1[BLOCKS_PER_LOOP];       ///< decoded data for the second channel
    int16_t *filterbuf[APE_FILTER_LEVELS];   ///< filter memory
    APERangecoder rc;                        ///< rangecoder used to decode actual values
    APERice riceX;                           ///< rice code parameters for the second channel
    APERice riceY;                           ///< rice code parameters for the first channel
    APEFilter filters[APE_FILTER_LEVELS][2]; ///< filters used for reconstruction
    uint32_t data_end;                       ///< frame data end
    uint32_t ptr;                      ///< current position in frame data
    int error;
    ByteCache dataobj;
} APEContextdec;

struct APEDecIn
{
    union
    {
        FILE *fd;
        struct
        {
            uint8_t *buf_in;
            uint32_t buf_in_size;
            uint32_t buf_in_left;
            uint32_t file_size;
            uint32_t file_tell;
        } buf;
    };
    int (*cb)(void *param);
    void *cb_param;
    uint32_t read_bytes;
};

typedef struct APEDec
{
    uint32_t malloc_size;
    APEContext *apeobj;
    APEContextdec *sobj;
    ByteIOContext *pbobj;
    int ID3_len;

    uint8_t *out_buf;
    uint32_t out_len;
    struct APEDecIn in;
} APEDec;

#endif

extern long ape_fsize(void *fd);
extern int ape_fseek(void *fd, int offset, int mode);
extern int ape_ftell(void *fd);
extern int ape_fread(uint8_t *buf, int size, int count, void *fd);

extern int ape_frame_prepare(APEDec *dec);
// extern int ape_read_header(APEDec *dec, struct play_ape *ape);
extern void init_ape(APEDec **dec);
extern void ape_decode(APEDec *dec);
extern void ape_free(APEDec *dec);
