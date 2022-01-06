#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>

#define ARGS_NUM 2
#define SERVER_PORT 10000
#define MAX_LEN 2048

#define LISTENQ 10

int main()
{
    char rbuf[MAX_LEN];
    fd_set allset;
    FD_ZERO(&allset);

    int i, listfd, ret, stin, maxfd, clifd, cliaddrLen;

//  init client fd table
    int clifds[FD_SETSIZE];
    for (i = 0; i < FD_SETSIZE; i++) {
        clifds[i] = -1;
    }
    // new socket
    struct sockaddr_in servaddr, cliaddr;
    bzero(&servaddr, sizeof(servaddr));
    bzero(&cliaddr, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // init socket
    listfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listfd <= 0) {
        perror("server listfd failed");
        exit(1);
    }
    // bind
    ret = bind(listfd, &servaddr, sizeof(servaddr));
    if (ret < 0) {
        perror("server bind faild");
        exit(1);
    }

    stin = fileno(stdin);
    //selset
    FD_SET(listfd, &allset);
    FD_SET(stin, &allset);
    //listen
    maxfd = listfd > stin ? listfd : stin;

    ret = listen(listfd, LISTENQ);

    while (1) {
        // blocking wait
        ret = select(maxfd + 1, &allset, NULL, NULL, NULL);
        if (ret > 0) {
            // listfd
            cliaddrLen = sizeof(cliaddr);
            // listfd accept and other fd read/write
            if (FD_ISSET(listfd, &allset)) {
                clifd = accept(listfd, &cliaddr, &cliaddrLen);
                if (clifd <= 0) {
                    perror("accept error");
                    exit(1);
                }

                for (i = 0; i < maxfd; i++) {
                    if (clifds[i] == -1) {
                        if (clifd > maxfd) {
                            maxfd = clifd;
                        }
                        FD_SET(clifd, &allset);
                        clifds[i] = clifd;
                        break;
                    }
                }
                if (i == maxfd) {
                    perror("connect out of bound");
                    exit(1);
                }
                if (--ret == 0) {
                    continue;
                }
            } else {
                // find client
                for (int i = 0; i < maxfd; i++) {
                    if (clifds[i] != -1 && FD_ISSET(clifds[i], &allset)) {
                        int sfd = clifds[i];
                        int n;
                        ret = read(sfd, rbuf, MAX_LEN);
                        if (ret <= 0) {
                            FD_CLR(sfd, &allset);
                            for (i = 0; i < maxfd; i++) {
                                if (clifds[i] == sfd) {
                                    clifds[i] = -1;
                                    break;
                                }
                            }
                            close(sfd);
                            continue;
                        }
                        n = write(sfd, rbuf, ret);
                        if (n != ret) {
                            perror("send failed");
                            exit(1);
                        }
                    }
                }

            }

        }

    }






}
