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
#include "../libws/ws.h"
#include "mux.h"

#define PORT 8888

char *name = "wsmux";

void process_message(char *message)
{
}

void server_message(struct mux_client *client, char *message)
{
    size_t len = strlen(message);

    // send websocket header
    char header[WS_FRAME_HEADER_SIZE];
    int header_len = ws_write_frame_header(header, WS_TEXT, len);
    if (sev_send(client->stream, header, header_len) == -1)
        return;

    // send data
    sev_send(client->stream, message, len);
}

void server_close(struct mux_client *client)
{
    sev_close(client->stream, "server_close");
}

static int header_cb(struct ws_header *header, void *data)
{
    struct mux_client *client = data;

    char buffer[WS_HTTP_RESPONSE_SIZE];
    ws_write_http_handshake(buffer, header->websocket_key);
    sev_send(client->stream, buffer, strlen(buffer));

    return 0;
}

static int frame_cb(struct ws_frame *frame, void *data)
{
    struct mux_client *client = data;

    mux_client_data(client, frame->chunk_data, frame->chunk_len);

    return 0;
}

static void open_cb(struct sev_stream *stream)
{
    struct mux_client *client = mux_client_open(stream);

    struct ws_parser *parser = malloc(sizeof(struct ws_parser));
    ws_parser_init(parser);

    parser->header_cb = header_cb;
    parser->frame_cb = frame_cb;

    parser->data = client;
    client->data = parser;
    stream->data = client;
}

static void read_cb(struct sev_stream *stream, char *data, size_t len)
{
    struct mux_client *client = stream->data;
    struct ws_parser *parser = client->data;

    if (ws_parse_all(parser, data, len) == -1)
        sev_close(stream, "websocket parse error");
}

static void close_cb(struct sev_stream *stream, const char *reason)
{
    struct mux_client *client = stream->data;
    struct ws_parser *parser = client->data;

    free(parser);
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
