/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

struct audio_player_queue *audio_queue_create(size_t item_count, size_t item_size)
{
    struct audio_player_queue *queue = (struct audio_player_queue *)audio_calloc(1, sizeof(*queue));
    int i;
    if (!queue)
        return NULL;
    memset(queue, 0x0, sizeof(*queue));
    queue->item_count = item_count;
    queue->item_size = item_size;
    queue->fill = 0;
    queue->read_pos = 0;
    queue->write_pos = 0;
    queue->state = 0;
    queue->item = &queue->item_temp;
    RK_AUDIO_LOG_D("sizeof(*queue) = %d.", sizeof(*queue));
    for (i = 0; i < item_count; i++)
    {
        queue->item[i] = audio_calloc(1, item_size);
        if (queue->item[i] == NULL)
            goto ERR;
    }
    queue->lock = audio_mutex_create();
    if (queue->lock == NULL)
        goto ERR;
    queue->read_sem = audio_semaphore_create();
    if (queue->read_sem == NULL)
        goto ERR;
    queue->write_sem = audio_semaphore_create();
    if (queue->write_sem == NULL)
        goto ERR;
    RK_AUDIO_LOG_D("item_count create =%x, %d.", queue, item_count);

    return queue;
ERR:
    RK_AUDIO_LOG_E("No enough memory");
    for (i = 0; i < item_count; i++)
    {
        if (queue->item[i])
            audio_free(queue->item[i]);
    }
    if (queue->lock)
        audio_mutex_destroy(queue->lock);
    if (queue->read_sem)
        audio_semaphore_destroy(queue->read_sem);
    if (queue->write_sem)
        audio_semaphore_destroy(queue->write_sem);
    audio_free(queue);

    return NULL;
}

int audio_queue_send(struct audio_player_queue *self, const void *data)
{
    while (1)
    {
        audio_mutex_lock(self->lock);
        RK_AUDIO_LOG_D("s:%x,self->fill:%d,self->item_count:%d", self, self->fill, self->item_count);
        if (self->state & AUDIO_QUEUE_STATE_STOPPED)
        {
            audio_mutex_unlock(self->lock);
            return RK_AUDIO_FAILURE;
        }
        if (self->fill != self->item_count)
        {
            break;
        }
        self->state |= AUDIO_QUEUE_STATE_WAIT_WRITABLE;
        audio_mutex_unlock(self->lock);
        audio_semaphore_take(self->write_sem);
    }
    RK_AUDIO_LOG_D("self->read_pos:%d", self->read_pos);
    memcpy(self->item[self->read_pos], data, self->item_size);
    self->fill++;
    self->read_pos++;
    self->read_pos = self->read_pos % self->item_count;
    if (self->state & AUDIO_QUEUE_STATE_WAIT_READABLE)
    {
        self->state &= (~AUDIO_QUEUE_STATE_WAIT_READABLE);
        audio_semaphore_give(self->read_sem);
    }
    audio_mutex_unlock(self->lock);
    return RK_AUDIO_SUCCESS;
}

int audio_queue_send_font(struct audio_player_queue *self, const void *data)
{
    return RK_AUDIO_SUCCESS;
}

int audio_queue_receive(struct audio_player_queue *self, void *data)
{
    while (1)
    {
        audio_mutex_lock(self->lock);
        RK_AUDIO_LOG_D("r:%x: self->fill:%d,self->item_count = %d", self, self->fill, self->item_count);
        RK_AUDIO_LOG_D("os_queue_state = %x.", (int)self->state);
        if (self->state & AUDIO_QUEUE_STATE_STOPPED)
        {
            RK_AUDIO_LOG_W("AUDIO_QUEUE_STATE_STOPPED.");
            audio_mutex_unlock(self->lock);
            return RK_AUDIO_FAILURE;
        }
        if (self->fill)
        {
            break;
        }
        if (self->state & AUDIO_QUEUE_STATE_FINISHED)
        {
            self->state &= (~AUDIO_QUEUE_STATE_FINISHED);
            audio_mutex_unlock(self->lock);
            RK_AUDIO_LOG_W("AUDIO_QUEUE_STATE_FINISHED.");
            return RK_AUDIO_SUCCESS;
        }
        self->state |= AUDIO_QUEUE_STATE_WAIT_READABLE;
        RK_AUDIO_LOG_D("os_queue_state = %x", (int)self->state);
        audio_mutex_unlock(self->lock);
        audio_semaphore_take(self->read_sem);
    }
    RK_AUDIO_LOG_D("self->write_pos:%d, size = %d", self->write_pos, self->item_size);
    memcpy(data, self->item[self->write_pos++], self->item_size);
    self->fill--;
    self->write_pos = self->write_pos % self->item_count;
    RK_AUDIO_LOG_D("self->write_pos:%d after", self->write_pos);
    if (self->state & AUDIO_QUEUE_STATE_WAIT_WRITABLE)
    {
        self->state &= (~AUDIO_QUEUE_STATE_WAIT_WRITABLE);
        audio_semaphore_give(self->write_sem);
    }
    audio_mutex_unlock(self->lock);
    return RK_AUDIO_SUCCESS;
}

int audio_queue_receive_back(struct audio_player_queue *self, void *data)
{
    return RK_AUDIO_SUCCESS;
}

int audio_queue_stop(struct audio_player_queue *self)
{
    audio_mutex_lock(self->lock);
    if (!(self->state | AUDIO_QUEUE_STATE_STOPPED))
    {
        if (self->state & AUDIO_QUEUE_STATE_WAIT_READABLE)
        {
            self->state &= (~AUDIO_QUEUE_STATE_WAIT_READABLE);
            audio_semaphore_give(self->read_sem);
        }
        if (self->state & AUDIO_QUEUE_STATE_WAIT_WRITABLE)
        {
            self->state &= (~AUDIO_QUEUE_STATE_WAIT_WRITABLE);
            audio_semaphore_give(self->write_sem);
        }
        self->state |= AUDIO_QUEUE_STATE_STOPPED;
    }
    audio_mutex_unlock(self->lock);
    return RK_AUDIO_SUCCESS;
}

bool audio_queue_is_full(struct audio_player_queue *self)
{
    bool is_full = false;
    audio_mutex_lock(self->lock);
    if (self->fill >= self->item_count)
    {
        is_full = true;
    }
    audio_mutex_unlock(self->lock);
    return is_full;
}

bool audio_queue_is_empty(struct audio_player_queue *self)
{
    bool is_empty = false;
    audio_mutex_lock(self->lock);
    if (self->fill == 0)
    {
        is_empty = true;
    }
    audio_mutex_unlock(self->lock);
    return is_empty;
}

int audio_queue_finish(struct audio_player_queue *self)
{
    audio_mutex_lock(self->lock);
    if (self->state & AUDIO_QUEUE_STATE_WAIT_READABLE)
    {
        self->state &= (~AUDIO_QUEUE_STATE_WAIT_READABLE);
        self->state |= AUDIO_QUEUE_STATE_FINISHED;
        audio_semaphore_give(self->read_sem);
    }
    else
    {
        self->state |= AUDIO_QUEUE_STATE_FINISHED;
    }
    audio_mutex_unlock(self->lock);
    return RK_AUDIO_SUCCESS;
}

int audio_queue_peek(struct audio_player_queue *self, void *data)
{
    audio_mutex_lock(self->lock);
    if (self->fill)
    {
        memcpy(data, self->item[self->write_pos], self->item_size);
    }
    audio_mutex_unlock(self->lock);
    return RK_AUDIO_SUCCESS;
}

void audio_queue_destroy(struct audio_player_queue *self)
{
    if (self)
    {
        audio_mutex_destroy(self->lock);
        audio_semaphore_destroy(self->read_sem);
        audio_semaphore_destroy(self->write_sem);
        while (self->item_count--)
        {
            audio_free(self->item[self->item_count]);
        }
        audio_free(self);
    }
}
