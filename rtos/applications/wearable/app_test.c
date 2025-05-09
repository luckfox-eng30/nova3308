/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#include <stdio.h>
#include <dfs_posix.h>

#include "drv_heap.h"
#include "hal_base.h"
#include "image_info.h"

#include "display.h"
#include "app_main.h"

int assert_test(void)
{
    RT_ASSERT(0);
    return 0;
}

MSH_CMD_EXPORT(assert_test, test);

int image_dec_test(int argc, char *argv[])
{
    struct app_main_data_t *pdata = app_main_data;
    image_info_t image;
    DIR *dir = NULL;
    struct dirent *dr = NULL;
    struct stat buf;
    FILE *fd;
    char *file = NULL;
    char *dir_path = NULL;
    char file_path[DFS_PATH_MAX * 2];
    rt_uint8_t *fb = NULL;
    uint32_t st, et;
    register rt_base_t level;
    int type = 1;

    void *(*heap_malloc)(rt_size_t size);
    void (*heap_free)(void *ptr);

    heap_malloc = rt_malloc_large;
    heap_free = rt_free_large;

    if (argc < 3)
    {
        rt_kprintf("Usage:\n"
                   "  %s <-f file>|<-d dir> [-m psram|sram]\n", argv[0]);
        return -1;
    }

    argv++;
    argc--;
    while (argc)
    {
        if (strcmp(*argv, "-f") == 0)
        {
            argc--;
            argv++;
            file = *argv;
        }
        else if (strcmp(*argv, "-d") == 0)
        {
            argc--;
            argv++;
            dir_path = *argv;
        }
        else if (strcmp(*argv, "-m") == 0)
        {
            argc--;
            argv++;
            if (*argv && strcmp(*argv, "psram") == 0)
            {
                heap_malloc = rt_malloc_psram;
                heap_free = rt_free_psram;
            }
        }
        argc--;
        argv++;
    }

    if (dir_path == NULL && file == NULL)
    {
        rt_kprintf("No target\n");
        return -1;
    }

    rt_kprintf("Use %s heap\n", heap_malloc == rt_malloc_large ? "SRAM" : "PSRAM");

    image.type  = IMG_TYPE_COMPRESS;
    image.pixel = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    image.x = 0;
    image.y = 0;
    image.w = 368;
    image.h = 368;
    fb = (rt_uint8_t *)heap_malloc(image.w * image.h);
    if (!fb)
    {
        rt_kprintf("fb NULL\n");
        return -1;
    }

#if APP_WDT_ENABLE
    if (pdata->wdt_dev)
        rt_device_control(pdata->wdt_dev, RT_DEVICE_CTRL_WDT_STOP, &type);
#endif
    level = rt_hw_interrupt_disable();
    if (dir_path != NULL)
    {
        dir = opendir(dir_path);
        if (NULL == dir)
        {
            rt_kprintf("open dir %s fail", dir_path);
            goto EXIT;
        }
        while (1)
        {
            dr = readdir(dir);
            if (dr != NULL)
            {
                if (!strcmp(dr->d_name, ".") || !strcmp(dr->d_name, ".."))
                    continue;

                if (dr->d_type == 1)
                {
                    memset(file_path, 0, sizeof(file_path));
                    snprintf(file_path, sizeof(file_path), "%s/%s%c", dir_path, dr->d_name, '\0');
                    rt_kprintf("Decompress %s.\n", file_path);
                    if (stat(file_path, &buf) == 0)
                    {
                        image.size = buf.st_size;

                        image.data = (rt_uint8_t *)heap_malloc(image.size);
                        if (image.data == NULL)
                        {
                            rt_kprintf("Malloc %d failed\n", image.size);
                            continue;
                        }
                        fd = fopen(file_path, "r");
                        fread(image.data, 1, image.size, fd);
                        st = HAL_GetTick();
                        rt_display_img_fill(&image, fb, image.w, 0, 0);
                        et = HAL_GetTick();
                        rt_kprintf("Cast %lu ms, file size %lu, w %d, h %d\n", et - st, image.size, image.w, image.h);
                        heap_free(image.data);
                        fclose(fd);
                    }
                    else
                    {
                        rt_kprintf("File empty %s.\n", file_path);
                        continue;
                    }
                }
            }
            else
            {
                break;
            }
        }
        closedir(dir);
    }
    if (file)
    {
        rt_kprintf("Decompress %s.\n", file);

        if (stat(file, &buf) == 0)
        {
            image.size = buf.st_size;

            image.data = (rt_uint8_t *)heap_malloc(image.size);
            if (image.data == NULL)
            {
                rt_kprintf("Malloc %d failed\n", image.size);
                goto EXIT;
            }
            fd = fopen(file, "r");
            fread(image.data, 1, image.size, fd);
            st = HAL_GetTick();
            rt_display_img_fill(&image, fb, image.w, 0, 0);
            et = HAL_GetTick();
            rt_kprintf("Cast %lu ms, file size %lu, w %d, h %d\n", et - st, image.size, image.w, image.h);
            heap_free(image.data);
            fclose(fd);
        }
        else
        {
            rt_kprintf("File empty %s.\n", file);
        }
    }
EXIT:
    rt_hw_interrupt_enable(level);
#if APP_WDT_ENABLE
    if (pdata->wdt_dev)
        rt_device_control(pdata->wdt_dev, RT_DEVICE_CTRL_WDT_START, &type);
#endif

    heap_free(fb);

    return 0;
}

MSH_CMD_EXPORT(image_dec_test, test);

void dialog_help(char *argv0)
{
    rt_kprintf("%s 0|2|4 helloworld\n", argv0);
    rt_kprintf("%s 1|3|5 93 174 /sdcard/icons/udisk_msc.dta\n", argv0);
}

void dialog_test_cb(enum dialog_cb_type type)
{
    rt_kprintf("%s\n", type == DIALOG_CALLBACK_OK ? "Dialog OK" : "Dialog Cancel");
}

extern struct app_page_data_t *app_cur_page;
void dialog_test(int argc, char *argv[])
{
    struct dialog_desc desc;
    int type;

    if (argc < 2)
    {
        dialog_help(argv[0]);
        return;
    }

    type = atoi(argv[1]);
    if (type == DIALOG_TEXT_OK_CANCEL ||
        type == DIALOG_TEXT_OK_ONLY ||
        type == DIALOG_TEXT_NO_CHECK)
    {
        if (argc < 3)
        {
            dialog_help(argv[0]);
            return;
        }
        desc.text = argv[2];
    }

    if (type == DIALOG_IMG_OK_CANCEL ||
        type == DIALOG_IMG_OK_ONLY ||
        type == DIALOG_IMG_NO_CHECK)
    {
        if (argc < 5)
        {
            dialog_help(argv[0]);
            return;
        }
        desc.img.w = atoi(argv[2]);
        desc.img.h = atoi(argv[3]);
        desc.img.name = argv[4];
    }
    desc.type = type;
    app_dialog_enter(app_cur_page, &desc, 1, dialog_test_cb);
}
MSH_CMD_EXPORT(dialog_test, dialog test);
