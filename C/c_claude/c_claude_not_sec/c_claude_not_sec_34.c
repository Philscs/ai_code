#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#endif

#define PORT 8080
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

void make_socket_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
}

int create_and_bind() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

int main() {
    int server_fd = create_and_bind();
    make_socket_non_blocking(server_fd);

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

#ifdef __linux__
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    struct epoll_event events[MAX_EVENTS];

    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl");
        close(server_fd);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == server_fd) {
                // Accept new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                make_socket_non_blocking(client_fd);
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    perror("epoll_ctl: client_fd");
                    close(client_fd);
                }
            } else {
                // Handle client data
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t count = read(events[i].data.fd, buffer, BUFFER_SIZE);
                if (count == -1) {
                    if (errno != EAGAIN) {
                        perror("read");
                        close(events[i].data.fd);
                    }
                } else if (count == 0) {
                    close(events[i].data.fd);
                } else {
                    write(events[i].data.fd, buffer, count); // Echo back
                }
            }
        }
    }

    close(epoll_fd);
#elif __APPLE__
    int kqueue_fd = kqueue();
    if (kqueue_fd == -1) {
        perror("kqueue");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    struct kevent change;
    struct kevent events[MAX_EVENTS];

    EV_SET(&change, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(kqueue_fd, &change, 1, NULL, 0, NULL) == -1) {
        perror("kevent");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        int n = kevent(kqueue_fd, NULL, 0, events, MAX_EVENTS, NULL);
        if (n == -1) {
            perror("kevent");
            break;
        }

        for (int i = 0; i < n; i++) {
            if (events[i].ident == server_fd) {
                // Accept new connection
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                make_socket_non_blocking(client_fd);
                EV_SET(&change, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
                if (kevent(kqueue_fd, &change, 1, NULL, 0, NULL) == -1) {
                    perror("kevent: client_fd");
                    close(client_fd);
                }
            } else {
                // Handle client data
                char buffer[BUFFER_SIZE];
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t count = read(events[i].ident, buffer, BUFFER_SIZE);
                if (count == -1) {
                    if (errno != EAGAIN) {
                        perror("read");
                        close(events[i].ident);
                    }
                } else if (count == 0) {
                    close(events[i].ident);
                } else {
                    write(events[i].ident, buffer, count); // Echo back
                }
            }
        }
    }

    close(kqueue_fd);
#endif

    close(server_fd);
    return 0;
}
