#ifndef __APP_PAGE_H__
#define __APP_PAGE_H__

enum app_page_id
{
    ID_NONE = 0,
    ID_MAIN,
    ID_MSG,
    ID_FUNCLIST,
    ID_FUNC_UPDN,
    ID_FUNC_LR,
};

enum
{
    EXIT_SIDE_NONE = 0,
    EXIT_SIDE_LEFT,
    EXIT_SIDE_RIGHT,
    EXIT_SIDE_TOP,
    EXIT_SIDE_BOTTOM,
};

struct app_page_ops_t
{
    void (*init)(void);
    void (*enter)(void);
    void (*change)(void);
    void (*leave)(void);
    void (*deinit)(void);
};

struct app_page_data_t
{
    uint8_t  id;
    uint32_t fblen;
    uint8_t  *fb;

    uint16_t w;
    uint16_t h;
    uint16_t vir_w;
    uint8_t  win_loop;

    int16_t  hor_page;
    int16_t  ver_page;
    int16_t  hor_offset;
    int16_t  ver_offset;
    uint16_t hor_step;
    uint16_t ver_step;

    uint8_t  exit_side;

    uint8_t  win_id;
    uint8_t  win_layer;
    rt_uint8_t  format;
    rt_uint32_t *lut;
    rt_uint32_t lutsize;
    rt_uint32_t new_lut;
    rt_uint32_t hide_win;

    // struct app_page_ops_t ops;
    struct app_touch_cb_t  *touch_cb;

    struct app_page_data_t *next;

    struct app_page_data_t *left;
    struct app_page_data_t *right;
    struct app_page_data_t *top;
    struct app_page_data_t *bottom;

    struct app_page_data_t *last;

    void   *private;
};

extern const uint8_t format2depth[RTGRAPHIC_PIXEL_FORMAT_ARGB565 + 1];

rt_err_t app_page_refresh(struct app_page_data_t *page, uint8_t page_num, uint8_t auto_resize);

#endif
