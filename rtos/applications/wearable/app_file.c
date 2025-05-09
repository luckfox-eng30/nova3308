/*
 * Copyright (c) 2020 Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "app_main.h"

#define DBG_SECTION_NAME    "FILE"
#define DBG_LEVEL           DBG_INFO
#include "rtdbg.h"

#define APP_INFO_FILE       USERDATA_PATH"sys.inf"

struct file_info
{
    char   target[MAX_FILE_NAME];
    size_t total_file;
    size_t cur_file;
};
static struct file_info g_info = {{0}, 0, 0};

void get_app_info(struct app_main_data_t *info)
{
    struct app_main_data_t app_info;
    FILE *fd;

    info->clock_style = 1;
    info->play_mode = APP_PLAY_LIST;
    info->bl_time = APP_TIMING_LIGHTOFF;
    info->bl = 50;
    info->play_vol = APP_PLAY_VOL_DEFAULT;
    fd = fopen(APP_INFO_FILE, "rb");
    if (!fd)
    {
        save_app_info(info);
        return;
    }
    fread((char *)&app_info, 1, sizeof(struct app_main_data_t), fd);

    if (app_info.clock_style >= 0 && app_info.clock_style < 4)
        info->clock_style = app_info.clock_style;

    if (app_info.play_mode >= APP_PLAY_LIST && app_info.play_mode <= APP_PLAY_RANDOM)
        info->play_mode = app_info.play_mode;

    if (app_info.bl_time >= APP_TIMING_LIGHTOFF_MIN)
        info->bl_time = app_info.bl_time;
    else
        info->bl_time = 0;

    if (app_info.bl >= 20 && app_info.bl <= 100)
        info->bl = app_info.bl;

    if (app_info.play_vol >= APP_PLAY_VOL_MIN && app_info.play_vol <= APP_PLAY_VOL_MAX)
        info->play_vol = app_info.play_vol;

    fclose(fd);
}

void save_app_info(struct app_main_data_t *info)
{
    FILE *fd;

    fd = fopen(APP_INFO_FILE, "wb+");
    if (!fd)
        return;
    fwrite((char *)info, 1, sizeof(struct app_main_data_t), fd);
    fclose(fd);
}

int rootfs_check(void)
{
    rt_device_t root_dev;
    int format = 0;

    root_dev = rt_device_find("root");
    RT_ASSERT(root_dev);
    if (dfs_filesystem_get_mounted_path(root_dev) == NULL)
    {
        LOG_W("root is not mounted");
        dfs_mkfs("elm", "root");
        if (dfs_mount("root", "/", "elm", 0, 0) < 0)
        {
            LOG_E("mount root failed");
            format |= ROOT_NO_MOUNTED;
        }
    }
#ifdef RT_SDCARD_MOUNT_POINT
    rt_device_t sd_dev;
    int retry = 0;
    if (access(RT_SDCARD_MOUNT_POINT, F_OK) < 0)
    {
        LOG_I("create sd mount point %s.", RT_SDCARD_MOUNT_POINT);
        mkdir(RT_SDCARD_MOUNT_POINT, 0);
    }

    /* Wait for sd0 be registed */
    do
    {
        sd_dev = rt_device_find("sd0");
        rt_thread_mdelay(10);
        retry++;
        if (retry > 100)
            break;
    }
    while (!sd_dev);
    if (sd_dev)
    {
        if (dfs_filesystem_get_mounted_path(sd_dev) == NULL)
        {
            if (dfs_mount("sd0", RT_SDCARD_MOUNT_POINT, "elm", 0, 0) < 0)
            {
                LOG_I("sd0 is not mounted");
                format |= SD0_NO_MOUNTED;
            }
        }
    }
#endif

    return format;
}

uint32_t check_audio_type(char *file_name)
{
    char *str;
    str = strstr(file_name, ".");
    if (str)
    {
        if (strcmp(str + 1, "wav") == 0)
            return RT_EOK;
        if (strcmp(str + 1, "mp3") == 0)
            return RT_EOK;
    }

    return -RT_ERROR;
}

char *get_audio(void)
{
    if (g_info.cur_file == 0)
        return NULL;

    return g_info.target;
}

static char *get_audio_by_index(uint32_t index)
{
    DIR *dir = NULL;
    struct dirent *dr = NULL;
    uint32_t cnt = 0;

    dir = opendir(MUSIC_DIR_PATH);
    if (NULL == dir)
    {
        LOG_E("open dir %s fail", MUSIC_DIR_PATH);
        return NULL;
    }

    while (cnt < index)
    {
        dr = readdir(dir);
        if (dr != NULL)
        {
            if (!strcmp(dr->d_name, ".") || !strcmp(dr->d_name, ".."))
                continue;

            if (dr->d_type == 1)
            {
                LOG_D("dr->d_name=%s", dr->d_name);
                if (check_audio_type(dr->d_name) == RT_EOK)
                    cnt++;
            }
        }
        else
        {
            break;
        }
    }
    closedir(dir);

    if (dr)
    {
        LOG_D("found file [%s]", dr->d_name);
        snprintf(g_info.target, sizeof(g_info.target),
                 "%s/%s%c", MUSIC_DIR_PATH, dr->d_name, '\0');

        return g_info.target;
    }

    return NULL;
}
#if 0
/* remove file and return left file count */
uint32_t remove_file(const char *path, char *file_name)
{
    char str[256];
    sprintf(str, "%s/%s", path, file_name);
    if (unlink(str) == 0)
        return scan_audio(path);

    return -RT_ERROR;
}
#endif
/* return audio file count in path */
void scan_audio(void)
{
    DIR *dir = NULL;
    struct dirent *dr = NULL;
    uint32_t cnt = 0;
    int ret;

    ret = access(MUSIC_DIR_PATH, F_OK);
    if (ret < 0)
    {
        mkdir(MUSIC_DIR_PATH, 0);
        g_info.total_file = 0;
        g_info.cur_file = 0;
        return;
    }

    dir = opendir(MUSIC_DIR_PATH);
    if (NULL == dir)
    {
        LOG_E("open dir %s fail", MUSIC_DIR_PATH);
        g_info.total_file = 0;
        g_info.cur_file = 0;
        return;
    }

    LOG_D("open dir %s success :%p", MUSIC_DIR_PATH, dir);
    while (1)
    {
        dr = readdir(dir);
        if (dr != NULL)
        {
            if (!strcmp(dr->d_name, ".") || !strcmp(dr->d_name, ".."))
                continue;

            LOG_D("d_name = %s dtype = %d", dr->d_name, dr->d_type);

            if (dr->d_type == 1)
            {
                LOG_D("dr->d_name=%s", dr->d_name);
                if (check_audio_type(dr->d_name) == RT_EOK)
                {
                    cnt++;
                    if (cnt == 1 && g_info.cur_file == 0)
                    {
                        g_info.cur_file = 1;
                        snprintf(g_info.target, sizeof(g_info.target),
                                 "%s/%s%c", MUSIC_DIR_PATH, dr->d_name, '\0');
                    }
                    /*
                    else if (cnt == g_info.cur_file)
                    {
                        snprintf(g_info.target, sizeof(g_info.target),
                                 "%s/%s%c", MUSIC_DIR_PATH, dr->d_name, '\0');
                    }
                    */
                }
            }
        }
        if (dr == NULL)
        {
            closedir(dir);
            break;
        }
    }

    LOG_D("PATH [%s]: have %ld files", MUSIC_DIR_PATH, cnt);
    g_info.total_file = cnt;
    if (g_info.cur_file > g_info.total_file)
    {
        g_info.cur_file = g_info.total_file;
    }
}

char *app_random_file(void)
{
    uint32_t index;
    char *target;

    if (g_info.total_file == 1)
        return g_info.target;

    do
    {
        index = rand() % g_info.total_file + 1;
    }
    while (g_info.cur_file == index);

    target = get_audio_by_index(index);
    if (target != NULL)
        g_info.cur_file = index;

    return target;
}

char *app_next_file(void)
{
    uint32_t index;
    char *target;

    if (g_info.total_file == 1)
        return g_info.target;

    if (g_info.cur_file < g_info.total_file)
        index = g_info.cur_file + 1;
    else
        index = 1;

    target = get_audio_by_index(index);
    if (target != NULL)
        g_info.cur_file = index;

    return target;
}

char *app_prev_file(void)
{
    uint32_t index;
    char *target;

    if (g_info.total_file == 1)
        return g_info.target;

    if (g_info.cur_file > 1)
        index = g_info.cur_file - 1;
    else
        index = g_info.total_file;

    target = get_audio_by_index(index);
    if (target != NULL)
        g_info.cur_file = index;

    return target;
}
