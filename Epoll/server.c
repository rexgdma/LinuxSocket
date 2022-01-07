#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>

#define MAX_CON 20
#define PORT 10001
#define LISTENQ 20
#define MAX_LEN 2048

void SetNonBlock(int fd)
{
	int opt;
	opt = fcntl(fd, F_GETFL);
	opt = opt | FNONBLOCK;
    fcntl(fd, F_SETFL, opt);
	// set ev;
}

int main()
{
    struct epoll_event ev, events[MAX_CON];
    struct sockaddr_in servaddr, cliaddr;
    int listfd, ret;
    int clifd, clilen;
    int epfd, nfds, n;
    // new socket 
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    char rbuf[MAX_LEN];

    listfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listfd <= 0) {
    	perror("new socket failed");
    	exit(0);
    }

    ret = bind(listfd, &servaddr, sizeof(servaddr));
    if (ret < 0) {
        perror("bind fd failed");
        exit(0);
    }

    ret = listen(listfd, LISTENQ);

    epfd = epoll_create(1);

    // add listfd to epoll
    ev.data.fd = listfd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listfd, &ev);

    while (1) {
    	nfds = epoll_wait(epfd, &events, 1, -1);
        for (int i = 0; i < nfds; i++) {
        	if (events[i].data.fd == listfd) {
        		clilen = sizeof(cliaddr);
                clifd = accept(listfd, (struct sockaddr*)&cliaddr, &clilen);
                if (clifd < 0) {
                	perror("accept error");
                	continue;
                }
                printf("accept fd[%d] success\n", clifd);
                // set sock NONBLOCK
                SetNonBlock(clifd);
                // add clifd to epoll
                ev.data.fd = clifd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clifd, &ev);
        	} else if (events[i].events & EPOLLIN) { // recive msg, how about sendmsg longger than recive buffer?
                memset(rbuf, 0 , MAX_LEN);
                if ((n = read(events[i].data.fd, rbuf, MAX_LEN)) < 0) {
                	close(events[i].data.fd);
                	events[i].data.fd = -1;
                }
                ev.data.fd = events[i].data.fd;
                ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
        	} else if (events[i].events & EPOLLOUT) {
                // send msg to client
                if ((n = write(events[i].data.fd, rbuf, strlen(rbuf))) != strlen(rbuf)) {
                	perror("wiret failed");
                	exit(0);
                }
                ev.data.fd = events[i].data.fd;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd, EPOLL_CTL_MOD, ev.data.fd, &ev);
                memset(rbuf, 0 , MAX_LEN);
        	}
        }

    }     
}