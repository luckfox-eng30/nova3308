/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#ifdef OS_IS_LINUX
audio_player_thread_handle audio_thread_create(char *name, uint32_t StackDeep, uint32_t Priority, audio_thread_cfg_t *cfg)
{
    pthread_t tid;

    if (pthread_create(&tid, NULL, (void *(*)(void *))cfg->run, cfg->args))
    {
        RK_AUDIO_LOG_E("Thread %s create failed", name);
        return NULL;
    }
    RK_AUDIO_LOG_D("Create thread %s", name);
    return (audio_player_thread_handle)tid;
}

void audio_thread_exit(audio_player_thread_handle self)
{
    pthread_join((pthread_t)self, NULL);
}

audio_player_semaphore_handle audio_semaphore_create(void)
{
    sem_t *sem;
    sem = (sem_t *)malloc(sizeof(sem_t));
    if (!sem)
    {
        RK_AUDIO_LOG_E("failed");
        return NULL;
    }
    sem_init(sem, 0, 1);
    return (audio_player_semaphore_handle)sem;
}

int audio_semaphore_take(audio_player_semaphore_handle self)
{
    return sem_wait((sem_t *)self);
}

int audio_semaphore_try_take(audio_player_semaphore_handle self)
{
    return sem_trywait((sem_t *)self);
}

int audio_semaphore_give(audio_player_semaphore_handle self)
{
    return sem_post((sem_t *)self);
}

void audio_semaphore_destroy(audio_player_semaphore_handle self)
{
    free((sem_t *)self);
}

audio_player_mutex_handle audio_mutex_create(void)
{
    pthread_mutex_t *mutex;
    mutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (!mutex)
    {
        RK_AUDIO_LOG_E("failed");
        return NULL;
    }
    pthread_mutex_init(mutex, NULL);

    return (audio_player_mutex_handle)mutex;
}

int audio_mutex_lock(audio_player_mutex_handle self)
{
    return pthread_mutex_lock((pthread_mutex_t *)self);
}

int audio_mutex_unlock(audio_player_mutex_handle self)
{
    return pthread_mutex_unlock((pthread_mutex_t *)self);
}

void audio_mutex_destroy(audio_player_mutex_handle self)
{
    pthread_mutex_destroy((pthread_mutex_t *)self);
    free((pthread_mutex_t *)self);
}

void *audio_timer_create(char *name, uint32_t period, uint32_t reload, void *param, void (*timer_callback)(void *))
{
    return NULL;
}

int audio_timer_control(void *timer, uint32_t new_period, uint32_t over_time)
{
    return 0;
}

int audio_timer_start(void *timer)
{
    return 0;
}

int audio_timer_stop(void *timer)
{
    return 0;
}

int audio_timer_delete(void *timer)
{
    return 0;
}

void *audio_malloc(size_t size)
{
    return malloc(size);
}

void audio_free(void *ptr)
{
    free(ptr);
}

void audio_free_uncache(void *ptr)
{
    free(ptr);
}

void *audio_calloc(size_t nmemb, size_t size)
{
    return calloc(nmemb, size);
}

void *audio_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void *audio_malloc_uncache(size_t size)
{
    return malloc(size);
}

void *audio_device_open(const int dev_id, int flag)
{
    void *audio_dev;

    return audio_dev;
}

int audio_device_control(void *dev, uint32_t cmd, void *arg)
{
    return 0;
}

int audio_device_close(void *dev)
{
    return 0;
}

unsigned long audio_device_write(void *dev, char *data, unsigned long frames)
{
    return 0;
}

unsigned long audio_device_read(void *dev, char *data, unsigned long frames)
{
    return 0;
}

int audio_device_set_vol(void *dev, int vol)
{
    return 0;
}

int audio_device_get_vol(void *dev)
{
    return 0;
}

void audio_device_set_gain(void *dev, int ch, int dB)
{

}

int audio_device_get_gain(void *dev, int ch)
{
    return 0;
}

void *audio_open_dsp(uint32_t freq)
{

}

int32_t audio_ctrl_dsp(void *dsp, int cmd, void *arg)
{
    return RK_AUDIO_FAILURE;
}

void audio_close_dsp(void *dsp)
{

}

void audio_sleep(uint32_t ms)
{
    usleep(ms * 1000);
}

void audio_cache_ops(int ops, void *addr, int size)
{

}

int audio_fopen(char *path, char *mode)
{
    FILE *audio_file = NULL;
    if (strstr(mode, "+"))
    {
        audio_file = fopen(path, "wb");
        if (audio_file <= 0)
        {
            RK_AUDIO_LOG_E("file open O_RDWR failed");
            return 0;
        }
    }
    else if (strstr(mode, "r"))
    {
        audio_file = fopen(path, "rb");
        if (audio_file <= 0)
        {
            RK_AUDIO_LOG_E("file open O_RDONLY faile");
            return 0;
        }
    }
    else if (strstr(mode, "w"))
    {
        audio_file = fopen(path, "wb");
        if (audio_file <= 0)
        {
            RK_AUDIO_LOG_E("file open O_WRONLY faile");
            return 0;
        }
    }
    return (int)audio_file;
}

int audio_fread(void *buffer, size_t size, size_t count, int stream)
{
    int ret = 0;

    if (buffer == NULL || size == 0 || count == 0)
    {
        RK_AUDIO_LOG_W("NO read data.");
        return 0;
    }

    ret = fread(buffer, size, count, (FILE *)stream);
    if (ret == -1)
        return 0;

    return ret;
}

int audio_fwrite(const void *buffer, size_t size, size_t count, int stream)
{
    int ret = 0;

    if (buffer == NULL || size == 0 || count == 0)
    {
        RK_AUDIO_LOG_W("NO write data.");
        return 0;
    }

    ret = fwrite(buffer, size, count, (FILE *)stream);
    if (ret != count)
        return 0;

    return ret;
}

int audio_fclose(int fd)
{
    return fclose((FILE *)fd);
}

int audio_fsync(int fd)
{
    FILE *fp = (FILE *)fd;
    //return fsync(fp->_fileno);
    return 0;
}

uint32_t audio_fstat(char *file_path, int stream)
{
    struct stat buf;
    if (stat(file_path, &buf) == 0)
        return buf.st_size;
    else
        return 0;
}

uint32_t audio_ftell(int stream)
{
    return ftell((FILE *)stream);
}

int audio_fseek(int stream, int32_t offset, uint32_t pos)
{
    int ret = fseek((FILE *)stream, offset, pos);
    if (ret < 0)
    {
        return RK_AUDIO_FAILURE;
    }
    return ret;
}
#endif
