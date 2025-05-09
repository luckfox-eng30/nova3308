/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

struct audio_player_stream *audio_stream_create(size_t size)
{
    struct audio_player_stream *stream = (struct audio_player_stream *) audio_calloc(1, sizeof(struct audio_player_stream));
    if (stream)
    {
        memset(stream, 0x0, sizeof(struct audio_player_stream));
        stream->buf_size = size;
        stream->buf = (char *)audio_calloc(1, size);
        if (stream->buf == NULL)
            goto ERR;
        stream->lock = audio_mutex_create();
        if (stream->lock == NULL)
            goto ERR;
        stream->read_sem = audio_semaphore_create();
        if (stream->read_sem == NULL)
            goto ERR;
        stream->write_sem = audio_semaphore_create();
        if (stream->write_sem == NULL)
            goto ERR;
    }
    return stream;
ERR:
    RK_AUDIO_LOG_E("No enough memory");
    if (stream->buf)
        audio_free(stream->buf);
    if (stream->lock)
        audio_mutex_destroy(stream->lock);
    if (stream->read_sem)
        audio_semaphore_destroy(stream->read_sem);
    if (stream->write_sem)
        audio_semaphore_destroy(stream->write_sem);
    audio_free(stream);

    return NULL;
}

int audio_stream_start(struct audio_player_stream *self)
{
    audio_mutex_lock(self->lock);
    self->fill = 0;
    self->read_pos = 0;
    self->write_pos = 0;
    self->state = AUDIO_STREAM_STATE_RUN;
    audio_mutex_unlock(self->lock);

    return RK_AUDIO_SUCCESS;
}

int audio_stream_read(struct audio_player_stream *self, char *data, size_t data_len)
{
    size_t buf_size;
    size_t fill;
    size_t read_pos = 0;
    size_t read_size = 0;
    size_t remaining_size = data_len;
    size_t total_read_size = 0;
    char *read_buf = data;
    if (data_len)
    {
        while (1)
        {
            audio_mutex_lock(self->lock);
            if (self->state & AUDIO_STREAM_STATE_STOPPED)
            {
                total_read_size = -1;
                audio_mutex_unlock(self->lock);
                break;
            }

            buf_size = self->buf_size;
            fill = self->fill;
            read_pos = self->read_pos;
            read_size = buf_size - read_pos;
            if (read_size > buf_size)
            {
                read_size = buf_size;
            }
            if (read_size >= remaining_size)
            {
                read_size = remaining_size;
            }
            if (read_size <= fill)
            {
                memcpy(read_buf, self->buf + read_pos, read_size);
                self->fill -= read_size;
                self->read_pos = (self->read_pos + read_size) % buf_size;
                if (self->state & AUDIO_STREAM_STATE_WAIT_WRITABLE)
                {
                    self->state &= (~AUDIO_STREAM_STATE_WAIT_WRITABLE);
                    audio_semaphore_give(self->write_sem);
                }
                remaining_size -= read_size;
                total_read_size += read_size;
                read_buf += read_size;
                audio_mutex_unlock(self->lock);
                if (!remaining_size)
                {
                    break;
                }
            }
            else if (self->state & AUDIO_STREAM_STATE_FINISHED)
            {
                if (!fill)
                {
                    audio_mutex_unlock(self->lock);
                    break;
                }
                read_size = fill;
                memcpy(read_buf, self->buf + read_pos, read_size);
                self->fill -= read_size;
                self->read_pos = (self->read_pos + read_size) % buf_size;
                remaining_size -= read_size;
                total_read_size += read_size;
                read_buf += read_size;
                audio_mutex_unlock(self->lock);//add by cherry
                break;
            }
            else
            {
                self->state |= AUDIO_STREAM_STATE_WAIT_READABLE;
                audio_mutex_unlock(self->lock);
                audio_semaphore_take(self->read_sem);
            }
        }
    }
    return total_read_size;
}

int audio_stream_read2(struct audio_player_stream *self, char *data, size_t data_len)
{
    return RK_AUDIO_SUCCESS;
}

int audio_stream_write(struct audio_player_stream *self, const char *data, size_t data_len)
{
    size_t buf_size;
    size_t fill;
    size_t write_pos;
    size_t write_size;
    size_t remaining_size = data_len;
    size_t total_write_size = 0;
    const char *write_buf = data;
    if (data_len)
    {
        while (1)
        {
            audio_mutex_lock(self->lock);
            if (!(self->state & AUDIO_STREAM_STATE_RUN) ||
                 (self->state & AUDIO_STREAM_STATE_RESET))
            {
                audio_mutex_unlock(self->lock);
                return 0;
            }
            if (self->state & AUDIO_STREAM_STATE_STOPPED)
            {
                total_write_size = -1;
                audio_mutex_unlock(self->lock);
                break;
            }
            if (self->state & AUDIO_STREAM_STATE_FINISHED)
            {
                total_write_size = 0;
                audio_mutex_unlock(self->lock);
                break;
            }

            buf_size = self->buf_size;
            fill = self->fill;
            write_pos = self->write_pos;
            write_size = buf_size - write_pos;
            if (write_size >= buf_size)
            {
                write_size = buf_size;
            }
            if (write_size >= remaining_size)
            {
                write_size = remaining_size;
            }
            if (write_size <= buf_size - fill)
            {
                memcpy(self->buf + write_pos, write_buf, write_size);
                self->fill += write_size;
                self->write_pos = (self->write_pos + write_size) % buf_size;
                if (self->state & AUDIO_STREAM_STATE_WAIT_READABLE)
                {
                    self->state &= (~AUDIO_STREAM_STATE_WAIT_READABLE);
                    audio_semaphore_give(self->read_sem);
                }
                remaining_size -= write_size;
                total_write_size += write_size;
                write_buf += write_size;
                audio_mutex_unlock(self->lock);
                if (!remaining_size)
                {
                    break;
                }
            }
            else
            {
                self->state |= AUDIO_STREAM_STATE_WAIT_WRITABLE;
                audio_mutex_unlock(self->lock);
                audio_semaphore_take(self->write_sem);
                if (self->state & AUDIO_STREAM_STATE_RESET)
                {
                    total_write_size = data_len;
                    break;
                }
            }
        }
    }
    return total_write_size;

}

int audio_stream_write2(struct audio_player_stream *self, const char *data, size_t data_len)
{
    return RK_AUDIO_SUCCESS;
}

int audio_stream_finish(struct audio_player_stream *self)
{
    audio_mutex_lock(self->lock);
    if (self->state & AUDIO_STREAM_STATE_WAIT_READABLE)
    {
        self->state &= (~AUDIO_STREAM_STATE_WAIT_READABLE);
        self->state |= (AUDIO_STREAM_STATE_FINISHED);
        audio_semaphore_give(self->read_sem);
    }
    else
    {
        self->state |= (AUDIO_STREAM_STATE_FINISHED);
    }
    audio_mutex_unlock(self->lock);
    return RK_AUDIO_SUCCESS;
}

int audio_stream_stop(struct audio_player_stream *self)
{
    audio_mutex_lock(self->lock);
    if (!(self->state & AUDIO_STREAM_STATE_STOPPED))
    {
        if (self->state & AUDIO_STREAM_STATE_WAIT_WRITABLE)
        {
            self->state &= (~AUDIO_STREAM_STATE_WAIT_WRITABLE);
            self->state |= AUDIO_STREAM_STATE_STOPPED;
            audio_semaphore_give(self->write_sem);
        }
        else
        {
            self->state |= AUDIO_STREAM_STATE_STOPPED;
        }
        if (self->state & AUDIO_STREAM_STATE_WAIT_READABLE)
        {
            self->state &= (~AUDIO_STREAM_STATE_WAIT_READABLE);
            audio_semaphore_give(self->read_sem);
        }

    }
    audio_mutex_unlock(self->lock);
    return RK_AUDIO_SUCCESS;
}

int audio_stream_stop2(struct audio_player_stream *self)
{
    return RK_AUDIO_SUCCESS;
}

int audio_stream_resume(struct audio_player_stream *self)
{
    audio_mutex_lock(self->lock);
    if (self->state & AUDIO_STREAM_STATE_RESET)
        self->state &= (~AUDIO_STREAM_STATE_RESET);
    audio_mutex_unlock(self->lock);

    return RK_AUDIO_SUCCESS;
}

int audio_stream_reset(struct audio_player_stream *self)
{
    audio_mutex_lock(self->lock);
    self->fill = 0;
    self->read_pos = 0;
    self->write_pos = 0;
    self->state |= AUDIO_STREAM_STATE_RESET;
    if (self->state & AUDIO_STREAM_STATE_FINISHED)
        self->state &= (~AUDIO_STREAM_STATE_FINISHED);
    if (self->state & AUDIO_STREAM_STATE_WAIT_WRITABLE)
    {
        self->state &= (~AUDIO_STREAM_STATE_WAIT_WRITABLE);
        audio_semaphore_give(self->write_sem);
    }
    audio_mutex_unlock(self->lock);

    return RK_AUDIO_SUCCESS;
}

void audio_stream_destroy(struct audio_player_stream *self)
{
    if (self)
    {
        audio_mutex_destroy(self->lock);
        audio_semaphore_destroy(self->read_sem);
        audio_semaphore_destroy(self->write_sem);
        audio_free(self->buf);
        audio_free(self);
    }
}
