/*-
 * Copyright (c) 2013, Lessandro Mariano
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <ev.h>
#include <sys/queue.h>
#include "redismq/redismq.h"
#include "sev/sev.h"

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379
#define REDIS_DB 0

#define PORT 5555
#define NAME "tcpmux"
#define MQ_OUT "kernel_"

struct rmq_context mq_out, mq_in;
struct sev_server server;

struct tm_client {
    char *tag;
    struct sev_stream *stream;
    struct sev_queue *queue;
};

struct tm_client *tm_client_new(struct sev_stream *stream)
{
    struct tm_client *client = malloc(sizeof(struct tm_client));

    asprintf(&client->tag, "%s:%s-%d", NAME, stream->remote_address,
        stream->remote_port);
    client->stream = stream;
    client->queue = sev_queue_new();

    return client;
}

void tm_client_free(struct tm_client *client)
{
    sev_queue_free(client->queue);
    free(client->tag);
    free(client);
}

void open_cb(struct sev_stream *stream)
{
    printf("open %s:%d\n", stream->remote_address, stream->remote_port);

    struct tm_client *client = tm_client_new(stream);
    stream->data = client;

    rmq_rpushf(&mq_out, "connect %s %s", client->tag, stream->remote_address);
}

void read_cb(struct sev_stream *stream, const char *data, size_t len)
{
    struct tm_client *client = stream->data;
    sev_queue_push_back(client->queue, data, len);

    printf("todo\n");

    sev_queue_free_head(client->queue);
}

void close_cb(struct sev_stream *stream)
{
    printf("close %s:%d\n", stream->remote_address, stream->remote_port);

    struct tm_client *client = stream->data;
    rmq_rpushf(&mq_out, "disconnect %s %s", client->tag, "reason");
    tm_client_free(client);
}

void blpop_cb(const char *msg)
{
    printf("received %s\n", msg);
}

void mq_init(void)
{
    rmq_init(&mq_out, REDIS_HOST, REDIS_PORT, REDIS_DB, "mq:" MQ_OUT);
    rmq_rpush(&mq_out, "reset " NAME " server restart");

    rmq_init(&mq_in, REDIS_HOST, REDIS_PORT, REDIS_DB, "mq:" NAME);
    rmq_blpop(&mq_in, blpop_cb);
}

int sev_init(void)
{
    if (sev_listen(&server, PORT)) {
        perror("sev_server_init");
        return -1;
    }

    server.open_cb = open_cb;
    server.read_cb = read_cb;
    server.close_cb = close_cb;

    return 0;
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    mq_init();
    if (sev_init() == -1)
        return -1;

    ev_loop(EV_DEFAULT_ 0);

    return 0;
}
