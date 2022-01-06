#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#define ARGS_NUM 2
#define SERVER_PORT 10000
#define MAX_LEN 2048

int readline(int fd, char *buf, int maxlen)
{
	char c;
	char *ptr = buf;
	int len;
	int ret;
	for (len = 0; len < maxlen - 1; len++) {
        ret = read(fd, &c, 1);
        if (ret == 1) {
        	*ptr = c;
        	ptr++;
        	if (c == '\n') { // read a line end
                break;
        	}
        } else {
        	// read failed
        	*ptr = '\0';
        	break;
        }
	}
	*ptr = '\0';
	return len;
}

void str_cli(int fd)
{
	int ret;
	char wbuf[MAX_LEN];
	char rbuf[MAX_LEN];
	int wlen = 0;
	int rlen = 0;
    while (1) {
    	// get string for stdin
        memset(wbuf, 0, MAX_LEN);
    	if (fgets(wbuf, MAX_LEN, stdin) == NULL) {
            perror("fgets from stdin failed");
            exit(1);
    	}
    	write(fd, wbuf, strlen(wbuf));
    	
        memset(rbuf, 0, MAX_LEN);
    	ret = readline(fd, rbuf, MAX_LEN);
    	if (ret <= 0) {
    		perror("read error");
    		exit(1);
    	}
    	fputs(rbuf, stdout);
    }
}

int main(int args, char** argv)
{
	int ret;
	int fd;
	if (args != ARGS_NUM) {
	    perror("usage: server ip address");
	    return 0;
	}
	// new socket
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERVER_PORT);
	ret = inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	if (ret <= 0) {
		perror("error ip address format");
		return 0;
	}
	// init socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    // connect socket
    ret = connect(fd, &servaddr, sizeof(servaddr));
    if (ret < 0) {
    	perror("connect error\n");
    	close(fd);
    	return 0;
    }
    // process

    str_cli(fd);

    // close
    close(fd);
    return 0;
}
