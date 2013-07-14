#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <ev.h>
#include "redismq/redismq.h"

#define ASSERT(cond, name) do { \
    if (!(cond)) { \
        perror(name); \
        exit(-1); \
    } \
} while (0);

static int create_server(int port)
{
    int sd = socket(PF_INET, SOCK_STREAM, 0);
    ASSERT(sd != -1, "socket");

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    ASSERT(bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != -1, "bind");
    ASSERT(listen(sd, 2) != -1, "listen");

    return sd;
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    ASSERT((revents & EV_ERROR) == 0, "revents");

    char buffer[1024];

    // Receive message from client socket
    ssize_t read = recv(watcher->fd, buffer, 1023, 0);

    if (read < 0) {
        perror("recv");
        return;
    }

    if (read == 0) {
        printf("disconnected\n");

        ev_io_stop(EV_DEFAULT_ watcher);
        free(watcher);
        return;
    }

    buffer[read] = '\0';
    printf("message: %s\n", buffer);

    send(watcher->fd, buffer, read, 0);
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    ASSERT((revents & EV_ERROR) == 0, "revents");

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int sd = accept(watcher->fd, (struct sockaddr*)&addr, &addr_len);
    ASSERT(sd != -1, "accept");

    printf("connected\n");

    struct ev_io *w_client = (struct ev_io*)malloc(sizeof(struct ev_io));
    ev_io_init(w_client, read_cb, sd, EV_READ);
    ev_io_start(EV_DEFAULT_ w_client);
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    int sd = create_server(5555);

    struct ev_io w_accept;

    ev_io_init(&w_accept, accept_cb, sd, EV_READ);
    ev_io_start(EV_DEFAULT_ &w_accept);

    ev_loop(EV_DEFAULT_ 0);

    return 0;
}
