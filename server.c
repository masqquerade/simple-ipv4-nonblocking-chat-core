#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#define PORT 1234
#define BACKLOG 20

int set_sock_nb(int);
int handle_req(int);

int main() 
{
    struct sockaddr_in serv_addr;
    struct pollfd fds[200];

    int nfds = 0;
    ssize_t msg_len;
    int reuse_addr = 1;
    int rc, current_size;
    int i, k;
    int client_fd;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htons(0);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) 
    {
        perror("socket() error");
        exit(1);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int)) < 0)
    {
        perror("setsockopt() error");
        exit(1);
    }

    socklen_t serv_addrlen = sizeof(serv_addr);
    if (bind(listen_fd, (struct sockaddr *)&serv_addr, serv_addrlen) < 0)
    {
        perror("bind() error");
        exit(1);
    }

    if (listen(listen_fd, BACKLOG) < 0)
    {
        perror("listen() error");
        exit(1);
    }

    set_sock_nb(listen_fd);
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds++;

    do 
    {
        rc = poll(fds, nfds, (3 * 60 * 1000));

        if (rc < 0)
        {
            perror("poll() error");
            exit(1);
        }

        current_size = nfds;

        for (i = 0; i < nfds; i++)
        {
            if (fds[i].revents == 0)
            {
                continue;
            }

            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == listen_fd)
                {
                    do
                    {
                        client_fd = accept(listen_fd, NULL, NULL);
                    }
                    while (client_fd < 0 && errno == EINTR);

                    if (client_fd < 0 && errno == EWOULDBLOCK)
                    {
                        break;
                    }

                    fds[nfds].fd = client_fd;
                    fds[nfds].events = POLLIN|POLLOUT;
                    nfds++;
                }
                else
                {
                    char buf[4096];

                    do
                    {   
                        rc = recv(fds[i].fd, buf, sizeof(buf), 0);
                    }
                    while (rc < 0 && errno == EINTR);

                    if (rc < 0 && errno == EWOULDBLOCK)
                    {
                        break;
                    }
                    
                    msg_len = rc;

                    for (k = 0; k < nfds; k++)
                    {   
                        if (fds[k].fd != listen_fd && fds[k].fd != fds[i].fd)
                        {
                            do
                            {   
                                rc = send(fds[k].fd, buf, msg_len, 0);
                            }
                            while (rc < 0 && errno == EINTR);

                            if (rc < 0 && errno == EWOULDBLOCK)
                            {
                                break;
                            }
                        }
                    }
                }
            }
        }

    } while (1);
}

int set_sock_nb(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl() error");
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
    {
        perror("fcntl() error");
        return -1;
    }

    return 0;
}

