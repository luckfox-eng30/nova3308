/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"
#ifdef AUDIO_USING_HTTP

#ifdef OS_IS_FREERTOS
#include "net/http/http_api.h"
typedef struct HTTP_RES_HEADER http_response_header;
#else
#include "http_api.h"
typedef struct http_res_header http_response_header;
#endif

unsigned char g_is_chunked = 0;
unsigned long http_totalsize;
#endif

RK_AUDIO_USED static int check_http_audio_type(char *target, char *type)
{
    if (!strcmp(target, "audio/mpeg") || !strcmp(target, "audio/mp3"))
    {
        strcpy(type, "mp3");
    }
    else if (!strcmp(target, "application/octet-stream"))
    {
        strcpy(type, "mp3");  /* consider echocloud platform just provide mp3 */
        RK_AUDIO_LOG_V("# Content-Type: = %s #", target);
    }
    else
    {
        strcpy(type, "null");
    }
    return RK_AUDIO_SUCCESS;
}

int http_preprocessor_init_impl(struct play_preprocessor *self,
                                play_preprocessor_cfg_t *cfg)
{
#ifndef AUDIO_USING_HTTP
    return RK_AUDIO_FAILURE;
#else
    bool bSucc = RK_AUDIO_FALSE;
    int mem_size = HTTP_DOWNLOAD_FILE_NAME_LENGTH;
    char *response = NULL;
    char * target;
    target = cfg->target;

REDO:
    response = (char *) audio_malloc(mem_size * sizeof(char));

    int http_socket = -1;
    if (cfg->seek_pos)
    {
        if (http_open(target, &response, &http_socket, HTTP_GET, NULL, cfg->seek_pos) != HTTP_STATUS_OK)
        {
            goto END;
        }
    }
    else
    {
        if (http_open(target, &response, &http_socket, HTTP_GET, NULL, 0) != HTTP_STATUS_OK)
        {
            goto END;
        }
    }

    g_is_chunked = RK_AUDIO_FALSE;
    http_response_header resp = parse_header(response);
    RK_AUDIO_LOG_D(">>>>http header parse success:<<<<");
    RK_AUDIO_LOG_V("\tHTTP reponse: %d", resp.status_code);
    if (resp.status_code != 200 && resp.status_code != 206)
    {
        if (resp.status_code == 301 || resp.status_code == 302) //301 Moved Permanently / 302 Found
        {
            if (response)
                audio_free(response);
            target = resp.httpRedirectURL.pParam;
            close(http_socket);
            goto REDO;
        }
        else
        {
            RK_AUDIO_LOG_E("file can't be download,status code: %d", resp.status_code);
            bSucc = RK_AUDIO_FALSE;
            goto END;
        }
    }
    RK_AUDIO_LOG_D("\tHTTP file type: %s", resp.content_type);

    g_is_chunked = is_chunked(resp);

    if (g_is_chunked)
    {
        RK_AUDIO_LOG_V("\tHTTP transfer-encoding: %s", resp.transfer_encoding);
        cfg->file_size = 0;
    }
    else
    {
        RK_AUDIO_LOG_V("\tHTTP file length: %ld byte", resp.content_length);
        http_totalsize = resp.content_length;
        cfg->file_size = http_totalsize;
    }
    if (!cfg->isOta)
    {
        check_http_audio_type(resp.content_type, cfg->type);
    }

    self->userdata = (void *)http_socket;
    cfg->frame_size = HTTP_PREPROCESSOR_FRAME_SIZE;
    RK_AUDIO_LOG_V("[%s] open native http ok, http: %s, audio type:%s",
                   cfg->tag,
                   target,
                   cfg->type);
    bSucc = RK_AUDIO_TRUE;

END:
    if (response)
        audio_free(response);
    if (!bSucc)
    {
        if (http_socket != -1)
        {
            close(http_socket);
            RK_AUDIO_LOG_W("****** close socket. ******");
        }
        return RK_AUDIO_FAILURE;
    }
    return RK_AUDIO_SUCCESS;
#endif
}

int http_preprocessor_read_impl(struct play_preprocessor *self, char *data,
                                size_t data_len)
{
#ifndef AUDIO_USING_HTTP
    return RK_AUDIO_FAILURE;
#else
    //RK_AUDIO_LOG_D("read data_len:%d",data_len);
    int rs = RK_AUDIO_FALSE;

    if ((int)self->userdata == -1)
    {
        return RK_AUDIO_FALSE;
    }

    if (g_is_chunked)
    {
        rs = http_socket_read_chunk((int)self->userdata, data, data_len);
    }
    else
    {
        if (http_totalsize == 0)
            return 0;

        if (data_len > http_totalsize)
            data_len = http_totalsize;
        rs = http_socket_read((int)self->userdata, data, data_len);

        if (rs > 0)
        {
            if (rs != data_len)
            {
                RK_AUDIO_LOG_D("read data underrun,data_len:%d has_read:%d", data_len, rs);
            }
            http_totalsize -= rs;
        }
        else if (rs == 0)
        {
            if (http_totalsize > 0)
            {
                RK_AUDIO_LOG_E("http read data fail");
                return -101; //http not end, return this val for app stop player
            }
            else
            {
                RK_AUDIO_LOG_V("http read data over");
            }
        }
        else if (rs == -1)
        {
            //socket error, if return -101, watch dog will reset system
            return 0;
        }

    }

    return rs;
#endif
}

void http_preprocessor_destroy_impl(struct play_preprocessor *self)
{
#ifdef AUDIO_USING_HTTP
    if (!self)
        return;
    if ((int)self->userdata == -1)
    {
        return;
    }

    int client_socket = (int)self->userdata;
    http_close(client_socket);
#endif
}

int http_preprocessor_seek_impl(struct play_preprocessor *self,
                                play_preprocessor_cfg_t *cfg)
{
    http_preprocessor_destroy_impl(self);
    return http_preprocessor_init_impl(self, cfg);
}
