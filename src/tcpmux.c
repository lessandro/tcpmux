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

#include <stdio.h>
#include <string.h>
#include "../sev/sev.h"
#include "mux.h"

#define PORT 5555

char *name = "tcpmux";

void server_message(struct mux_client *client, char *message)
{
    size_t len = strlen(message);

    // send data
    sev_send(client->stream, message, len);
}

void server_close(struct mux_client *client)
{
    sev_close(client->stream, "server_close");
}

static void open_cb(struct sev_stream *stream)
{
    struct mux_client *client = mux_client_open(stream);
    stream->data = client;
}

static void read_cb(struct sev_stream *stream, char *data, size_t len)
{
    struct mux_client *client = stream->data;

    mux_client_data(client, data, len);
}

static void close_cb(struct sev_stream *stream, const char *reason)
{
    struct mux_client *client = stream->data;

    mux_client_close(client, reason);
}

int main(int argc, char *argv[])
{
    mux_init();

    struct sev_server server;
    if (sev_listen(&server, PORT)) {
        perror("sev_server_init");
        return -1;
    }

    server.open_cb = open_cb;
    server.read_cb = read_cb;
    server.close_cb = close_cb;

    printf("%s started on port %d\n", name, PORT);

    sev_loop();

    return 0;
}
