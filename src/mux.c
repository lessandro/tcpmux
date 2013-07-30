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

// for asprintf
#define _GNU_SOURCE 1

#include <stdio.h>
#include <string.h>
#include "../redismq/redismq.h"
#include "mux.h"

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379
#define REDIS_DB 7

extern char *name;
extern void server_message(struct mux_client *, char *message);
extern void server_close(struct mux_client *);
extern void process_message(char *message);

static struct rmq_context mq_out;
static struct rmq_context mq_in;

static int mux_client_cmp(struct mux_client *e1, struct mux_client *e2)
{
    return strcmp(e1->tag, e2->tag);
}

RB_HEAD(mux_client_tree, mux_client) head = RB_INITIALIZER(&head);
RB_GENERATE(mux_client_tree, mux_client, entry, mux_client_cmp);

static struct mux_client *mux_client_new(struct sev_stream *stream)
{
    struct mux_client *client = malloc(sizeof(struct mux_client));

    asprintf(&client->tag, "%s:%s-%d", name, stream->remote_address,
        stream->remote_port);
    client->stream = stream;
    sprintf(client->buffer, "message %s ", client->tag);
    client->buffer_len = strlen(client->buffer);
    client->buffer_start = client->buffer_len;

    RB_INSERT(mux_client_tree, &head, client);

    return client;
}

static void mux_client_free(struct mux_client *client)
{
    RB_REMOVE(mux_client_tree, &head, client);

    free(client->tag);
    free(client);
}

struct mux_client *mux_client_open(struct sev_stream *stream)
{
    struct mux_client *client = mux_client_new(stream);

    printf("open %s\n", client->tag);

    rmq_rpushf(&mq_out, "connect %s %s", client->tag,
        client->stream->remote_address);

    return client;
}

static int find_delimiter(const char *data, size_t len)
{
    for (int i = 0; i < len; i++)
        if (data[i] == '\n')
            return i;

    return -1;
}

void mux_client_data(struct mux_client *client, char *data, size_t len)
{
    while (len != 0) {
        int pos = find_delimiter(data, len);

        int n = pos == -1 ? len : pos;

        int rest = (BUFFER_SIZE - 1) - client->buffer_len;
        if (n > rest)
            n = rest;

        // append data (up to \n) to the buffer
        memcpy(client->buffer + client->buffer_len, data, n);
        client->buffer_len += n;
        client->buffer[client->buffer_len] = '\0';

        // did not find a \n -- wait for the rest of the message
        if (pos == -1)
            return;

        // terminate string on the first \r
        char *p = strchr(client->buffer, '\r');
        if (p)
            *p = '\0';

        printf("%s\n", client->buffer);
        rmq_rpush(&mq_out, client->buffer);
        client->buffer_len = client->buffer_start;

        // discard data before the delimiter, and the delimiter
        data += pos + 1;
        len -= pos + 1;
    }
}

void mux_client_close(struct mux_client *client, const char *reason)
{
    printf("close %s %s\n", client->tag, reason);

    rmq_rpushf(&mq_out, "disconnect %s %s", client->tag, reason);

    mux_client_free(client);
}

static void blpop_cb(char *reply)
{
    char *tags = reply;
    char *message = strchr(tags, ' ');
    *message++ = '\0';

    process_message(message);

    char *tag = strtok(tags, ",");
    for (; tag != NULL; tag = strtok(NULL, ",")) {
        struct mux_client key = { .tag = tag };
        struct mux_client *client = RB_FIND(mux_client_tree, &head, &key);

        if (client == NULL)
            continue;

        if (!*message) {
            server_close(client);
            continue;
        }

        server_message(client, message);
    }
}

void mux_init(void)
{
    rmq_init(&mq_out, REDIS_HOST, REDIS_PORT, REDIS_DB, "mq:kernel");
    rmq_rpushf(&mq_out, "reset %s server restart", name);

    char *mq;
    asprintf(&mq, "mq:%s", name);

    rmq_init(&mq_in, REDIS_HOST, REDIS_PORT, REDIS_DB, mq);
    rmq_blpop(&mq_in, blpop_cb);

    free(mq);
}
