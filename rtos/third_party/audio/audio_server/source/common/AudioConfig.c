/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#if defined(OS_IS_FREERTOS)
audio_player_thread_handle audio_thread_create(char *name, uint32_t StackDeep, uint32_t Priority, audio_thread_cfg_t *cfg)
{
    RK_AUDIO_LOG_D("audio_thread_create.");
    RK_TASK_CLASS *handle;
    handle = (RK_TASK_CLASS *)rktm_create_task((pTaskFunType)cfg->run, NULL, NULL, name, StackDeep, Priority, (void *)cfg->args);
    handle->suspend_mode = DISABLE_MODE;

    return (audio_player_thread_handle)handle;
}

void audio_thread_exit(audio_player_thread_handle self)
{
    rktm_delete_task((HTC)self);
}

audio_player_semaphore_handle audio_semaphore_create(void)
{
    return rkos_semaphore_create(1, 0);
}

int audio_semaphore_try_take(audio_player_semaphore_handle self)
{
    return rkos_semaphore_take(self, 0);
}

int audio_semaphore_take(audio_player_semaphore_handle self)
{
    return rkos_semaphore_take(self, -1);
}

int audio_semaphore_give(audio_player_semaphore_handle self)
{
    return rkos_semaphore_give(self);
}

void audio_semaphore_destroy(audio_player_semaphore_handle self)
{
    rkos_semaphore_delete(self);
}

audio_player_mutex_handle audio_mutex_create(void)
{
    return rkos_mutex_create();
}

int audio_mutex_lock(audio_player_mutex_handle self)
{
    return rkos_semaphore_take(self, MAX_DELAY);
}

int audio_mutex_unlock(audio_player_mutex_handle self)
{
    return rkos_semaphore_give(self);
}

void audio_mutex_destroy(audio_player_mutex_handle self)
{
    rkos_semaphore_delete(self);
}

void *audio_timer_create(char *name, uint32_t period, uint32_t reload, void *param, void (*timer_callback)(void *))
{
    return rkos_create_timer(period, reload, param, timer_callback);
}

int audio_timer_control(void *timer, uint32_t new_period, uint32_t over_time)
{
    return rkos_mod_timer(timer, new_period, over_time);
}

int audio_timer_start(void *timer)
{
    return rkos_start_timer(timer);
}

int audio_timer_stop(void *timer)
{
    return rkos_stop_timer(timer);
}

int audio_timer_delete(void *timer)
{
    return rkos_delete_timer(timer);
}

void *audio_malloc(size_t size)
{
    return rkos_memory_malloc(size);
}

void audio_free(void *ptr)
{
    rkos_memory_free(ptr);
}

void audio_free_uncache(void *ptr)
{
    rkos_memory_free_align(ptr);
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
    return rkos_memory_malloc_align(size, 128);
}

void *audio_device_open(const int dev_id, int flag)
{
    DEVICE_CLASS *audio_dev;
    int class_id = rkos_audio_get_class();
    if (class_id == -1)
    {
        RK_AUDIO_LOG_E("can not find audio class.");
        return NULL;
    }
    audio_dev = (DEVICE_CLASS *)rkdev_open(class_id, dev_id, 0x00);
    if (audio_dev == NULL)
    {
        RK_AUDIO_LOG_E("can not open audio card %d.", dev_id);
        return NULL;
    }

    return audio_dev;
}

int audio_device_control(void *dev, uint32_t cmd, void *arg)
{
    return rkdev_control(dev, cmd, arg);
}

int audio_device_close(void *dev)
{
    return rkdev_close(dev);
}

unsigned long audio_device_write(void *dev, char *data, unsigned long frames)
{
    return rkdev_write(dev, 0, data, frames);
}

unsigned long audio_device_read(void *dev, char *data, unsigned long frames)
{
    return rkdev_read(dev, 0, data, frames);
}

int audio_device_set_vol(void *dev, int vol)
{
    SystemSetVol(dev, vol);

    return RK_AUDIO_SUCCESS;
}

int audio_device_get_vol(void *dev)
{
    return SystemGetVol(dev);
}

void audio_device_set_gain(void *dev, int ch, int dB)
{
    SystemSetGain(dev, ch, dB);
}

int audio_device_get_gain(void *dev, int ch)
{
    return SystemGetGain(dev, ch);
}

void *audio_open_dsp(uint32_t freq)
{
    void *dev = NULL;

#ifdef AUDIO_USING_DSP
    if (rkdev_find(DEV_CLASS_DSP, 0) == NULL)
    {
        if (rkdev_create(DEV_CLASS_DSP, 0, NULL) != RK_AUDIO_SUCCESS)
        {
            RK_AUDIO_LOG_E("dsp create fail");
            return NULL;
        }
    }
    dev = rkdev_open(DEV_CLASS_DSP, 0, 0);
    if (dev == NULL)
    {
        RK_AUDIO_LOG_E("device dsp%d open fail", 0);
        rkdev_delete(DEV_CLASS_DSP, 0, NULL);
        return NULL;
    }
    if (rk_dsp_open(dev, 0))
    {
        RK_AUDIO_LOG_E("dsp open fail");
        rkdev_close(dev);
        rkdev_delete(DEV_CLASS_DSP, 0, NULL);
        return NULL;
    }
    if (freq)
        rk_dsp_control(dev, RKDSP_CTL_SET_FREQ, (void *)freq);
#endif

    return dev;
}

int32_t audio_ctrl_dsp(void *dsp, int cmd, void *arg)
{
#ifdef AUDIO_USING_DSP
    return rk_dsp_control(dsp, cmd, arg);
#else
    return RK_AUDIO_FAILURE;
#endif
}

void audio_close_dsp(void *dsp)
{
#ifdef AUDIO_USING_DSP
    rk_dsp_close(dsp);
    rkdev_close(dsp);
    if (rkdev_delete(DEV_CLASS_DSP, 0, NULL) != RK_AUDIO_SUCCESS)
    {
        RK_AUDIO_LOG_E("dsp delete fail");
    }
#endif
}

void audio_sleep(uint32_t ms)
{
    rkos_sleep(ms);
}

#elif defined(OS_IS_RTTHREAD)
audio_player_thread_handle audio_thread_create(char *name, uint32_t StackDeep, uint32_t Priority, audio_thread_cfg_t *cfg)
{
    RK_AUDIO_LOG_D("audio_thread_create.");
    rt_thread_t tid;
    tid = rt_thread_create(name, cfg->run, cfg->args, StackDeep, Priority, 10);
    if (tid)
        rt_thread_startup(tid);
    return (audio_player_thread_handle)tid;
}

void audio_thread_exit(audio_player_thread_handle self)
{
    rt_thread_delete((rt_thread_t)self);
}

audio_player_semaphore_handle audio_semaphore_create(void)
{
    return (audio_player_semaphore_handle)rt_sem_create("os_sem", 1, RT_IPC_FLAG_PRIO);
}

int audio_semaphore_take(audio_player_semaphore_handle self)
{
    return rt_sem_take(self, RT_WAITING_FOREVER);
}

int audio_semaphore_try_take(audio_player_semaphore_handle self)
{
    return rt_sem_take(self, 0);
}

int audio_semaphore_give(audio_player_semaphore_handle self)
{
    return rt_sem_release(self);
}

void audio_semaphore_destroy(audio_player_semaphore_handle self)
{
    rt_sem_delete(self);
}

audio_player_mutex_handle audio_mutex_create(void)
{
    return (audio_player_mutex_handle)rt_mutex_create("os_mutex", RT_IPC_FLAG_PRIO);
}

int audio_mutex_lock(audio_player_mutex_handle self)
{
    return rt_mutex_take((rt_mutex_t)self, RT_WAITING_FOREVER);
}

int audio_mutex_unlock(audio_player_mutex_handle self)
{
    return rt_mutex_release((rt_mutex_t)self);
}

void audio_mutex_destroy(audio_player_mutex_handle self)
{
    rt_mutex_delete((rt_mutex_t)self);
}

void *audio_timer_create(char *name, uint32_t period, uint32_t reload, void *param, void (*timer_callback)(void *))
{
    return rt_timer_create(name, timer_callback, param, period, reload);
}

int audio_timer_control(void *timer, uint32_t new_period, uint32_t over_time)
{
    return rt_timer_control(timer, RT_TIMER_CTRL_SET_TIME, &new_period);
}

int audio_timer_start(void *timer)
{
    return rt_timer_start(timer);
}

int audio_timer_stop(void *timer)
{
    return rt_timer_stop(timer);
}

int audio_timer_delete(void *timer)
{
    return rt_timer_delete(timer);
}

void *audio_malloc(size_t size)
{
    return rt_malloc(size);
}

void audio_free(void *ptr)
{
    rt_free(ptr);
}

void audio_free_uncache(void *ptr)
{
    rt_free_uncache(ptr);
}

void *audio_calloc(size_t nmemb, size_t size)
{
    return rt_calloc(nmemb, size);
}

void *audio_realloc(void *ptr, size_t size)
{
    return rt_realloc(ptr, size);
}

void *audio_malloc_uncache(size_t size)
{
    return rt_malloc_uncache(size);
}

void *audio_device_open(const int dev_id, int flag)
{
    struct rt_device *audio_dev;
    RK_AUDIO_LOG_D("Try to find audio device %s.", (char *)dev_id);
    audio_dev = rt_device_find((char *)dev_id);
    if (audio_dev == RT_NULL)
    {
        RK_AUDIO_LOG_E("can not find audio device %s.", (char *)dev_id);
        return NULL;
    }
    int ret = rt_device_open(audio_dev, flag);
    if (ret)
    {
        RK_AUDIO_LOG_E("can not open audio device %s %d.", (char *)dev_id, ret);
        return NULL;
    }

    return audio_dev;
}

int audio_device_control(void *dev, uint32_t cmd, void *arg)
{
    return rt_device_control((rt_device_t)dev, cmd, arg);
}

int audio_device_close(void *dev)
{
    return rt_device_close((rt_device_t)dev);
}

unsigned long audio_device_write(void *dev, char *data, unsigned long frames)
{
    return rt_device_write(dev, 0, data, frames);
}

unsigned long audio_device_read(void *dev, char *data, unsigned long frames)
{
    return rt_device_read(dev, 0, data, frames);
}

int audio_device_set_vol(void *dev, int vol)
{
    struct AUDIO_GAIN_INFO gainInfo;
    struct AUDIO_DB_CONFIG db_config;
    snd_softvol_t softvol;
    int ret;

    softvol.vol_l = vol;
    softvol.vol_r = vol;
    ret = rt_device_control(dev, RK_AUDIO_CTL_PLUGIN_SET_SOFTVOL, &softvol);
    if (ret == RK_AUDIO_SUCCESS)
        return RK_AUDIO_SUCCESS;

    ret = rt_device_control(dev, RK_AUDIO_CTL_GET_GAIN_INFO, &gainInfo);
    if (ret == RK_AUDIO_SUCCESS && gainInfo.step != 0)
    {
        int dB;

        if (vol > 100)
            vol = 100;
        if (vol < 0)
            vol = 0;
        RK_AUDIO_LOG_D("Get Max dB %ld, Min dB %ld, Step %ld", gainInfo.maxdB, gainInfo.mindB, gainInfo.step);
        if (vol == 0)
            dB = gainInfo.mindB;
        else
            dB = audio_persent_to_dB(gainInfo.mindB, gainInfo.maxdB, vol) / gainInfo.step * gainInfo.step;
        db_config.dB = dB;
        db_config.ch = 0;
        RK_AUDIO_LOG_D("Set %d of 100, equal to %d dB", vol, db_config.dB);
        ret = rt_device_control(dev, RK_AUDIO_CTL_SET_GAIN, &db_config);
        ret = rt_device_control(dev, RK_AUDIO_CTL_GET_GAIN, &db_config);
        if (db_config.dB != dB)
        {
            RK_AUDIO_LOG_D("Set %d dB failed\n", dB);
            return RK_AUDIO_FAILURE;
        }
    }

    return ret;
}

int audio_device_get_vol(void *dev)
{
    snd_softvol_t softvol;
    int ret;

    ret = rt_device_control(dev, RK_AUDIO_CTL_PLUGIN_GET_SOFTVOL, &softvol);
    if (ret == RK_AUDIO_SUCCESS)
        return softvol.vol_l;

    return RK_AUDIO_FAILURE;
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
    struct rt_device *dev = NULL;

#ifdef AUDIO_USING_DSP
    rt_err_t ret;
    dev = rt_device_find("dsp0");
    if (dev == NULL)
    {
        RK_AUDIO_LOG_E("cannot find dsp0");
        return NULL;
    }
    ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR);
    if (ret)
    {
        RK_AUDIO_LOG_E("cannot open dsp0");
        return NULL;
    }
    if (freq)
        rt_device_control(dev, RKDSP_CTL_SET_FREQ, (void *)freq);
#endif

    return (void *)dev;
}

int32_t audio_ctrl_dsp(void *dsp, int cmd, void *arg)
{
#ifdef AUDIO_USING_DSP
    return rt_device_control((struct rt_device *)dsp, cmd, arg);
#else
    return RK_AUDIO_FAILURE;
#endif
}

void audio_close_dsp(void *dsp)
{
#ifdef AUDIO_USING_DSP
    struct rt_device *dev = (struct rt_device *)dsp;
    rt_device_close(dev);
#endif
}

void audio_sleep(uint32_t ms)
{
    rt_thread_mdelay(ms);
}
#endif

int audio_persent_to_dB(int mindB, int maxdB, int persent)
{
    return ((log(persent) / log(100)) * (maxdB - mindB) + mindB);
}

void audio_cache_ops(int ops, void *addr, int size)
{
#ifdef OS_IS_FREERTOS
    rk_dcache_ops(ops, addr, size);
#else
    rt_hw_cpu_dcache_ops(ops, addr, size);
#endif
}

int audio_fopen(char *path, char *mode)
{
#if defined(OS_IS_FREERTOS)
    void *FileFd = NULL;

    FileFd = rkos_file_open(path, mode);
    RK_ASSERT((int)FileFd <= 0x7fffffff);
    if (FileFd <= 0)
    {
        return (int)NULL;
    }
    else
    {
        return (int)FileFd;
    }
#else
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
#endif
}

int audio_fread(void *buffer, size_t size, size_t count, int stream)
{
#if defined(OS_IS_FREERTOS)
    int ret = 0;
    if (stream == -1)
    {
        RK_AUDIO_LOG_E("audio_file = %d", stream);
        return 0;
    }
    ret = rkos_file_read(buffer, size, count, (HDC)stream);
    if (ret <= 0)
        return 0;
    return ret;
#else
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
#endif
}

int audio_fwrite(const void *buffer, size_t size, size_t count, int stream)
{
#if defined(OS_IS_FREERTOS)
    return rkos_file_write(buffer, size, count, (HDC)stream);
#else
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
#endif
}

int audio_fclose(int fd)
{
#if defined(OS_IS_FREERTOS)
    if (rkos_file_close((HDC)fd) != RK_SUCCESS)
    {
        return RK_AUDIO_FAILURE;
    }
    else
    {
        return RK_AUDIO_SUCCESS;
    }
#else
    return fclose((FILE *)fd);
#endif
}

int audio_fsync(int fd)
{
#if defined(OS_IS_FREERTOS)
    return 0;
#else
    FILE *fp = (FILE *)fd;
    return fsync(fp->_file);
#endif
}

uint32_t audio_fstat(char *file_path, int stream)
{
#if defined(OS_IS_FREERTOS)
    return rkos_file_get_size((HDC)stream);
#else
    struct stat buf;
    if (stat(file_path, &buf) == 0)
        return buf.st_size;
    else
        return 0;
#endif
}

uint32_t audio_ftell(int stream)
{
#if defined(OS_IS_FREERTOS)
    /* Not support */
    return 0;
#else
    return ftell((FILE *)stream);
#endif
}

int audio_fseek(int stream, int32_t offset, uint32_t pos)
{
#if defined(OS_IS_FREERTOS)
    int ret = rkos_file_lseek((HDC)stream, offset, pos);
    if (ret != RK_AUDIO_SUCCESS)
    {
        return RK_AUDIO_FAILURE;
    }
    return ret;
#else
    int ret = fseek((FILE *)stream, offset, pos);
    if (ret < 0)
    {
        return RK_AUDIO_FAILURE;
    }
    return ret;
#endif
}

