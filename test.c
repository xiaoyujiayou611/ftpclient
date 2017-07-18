#include "ftpclient.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/socket.h"
#include "arpa/inet.h"
#include "time.h"
#include "unistd.h"

#define BUFFERSIZE 256


int getport(char * str) {
    int i, j;
    char *ptr1, *ptr2;
    char num[BUFFERSIZE];
    bzero(num, BUFFERSIZE);
    //取低位字节
    ptr1 = str + strlen(str);
    while (*(ptr1) != ')') {
        ptr1--;
    }
    ptr2 = ptr1;
    while (*(ptr2) != ',')
        ptr2--;
    strncpy(num, ptr2 + 1, ptr1 - ptr2 - 1);
    i = atoi(num);//将字符串转换成整数
    //取高位字节
    bzero(num, BUFFERSIZE);
    ptr1 = ptr2;
    ptr2--;
    while (*(ptr2) != ',')
        ptr2--;
    strncpy(num, ptr2 + 1, ptr1 - ptr2 - 1);
    j = atoi(num);
    return j * 256 + i;
}


void sendcmd(ftpFd fd, char * cmd, char * code, char * ret) {
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);

    snprintf(sendline, BUFFERSIZE, "%s\r\n", cmd);
    printf(">>>%s\n", cmd);
    if (send(fd, sendline, strlen(sendline), 0) < 0) {
        printf("Send cmd %s error\n", cmd);
        exit(1);
    }
    if (recv(fd, recvline, sizeof(recvline), 0) < 0) {
        printf("Recv cmd %s error\n", cmd);
        exit(1);
    }
    if (strncmp(recvline, code, 3) == 0) {
        printf("%s\n", recvline);
    } else {
        printf("Exec cmd %s error: %s\n", cmd, recvline);
        exit(1);
    }
}


int speed(ftpFd control_sockfd, char * addr, clock_t seconds) {
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    int dataport;
    ftpFd data_sockfd;
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);

    // binary mode
    sendcmd(control_sockfd, "TYPE I", "200", recvline);
    

    //PASV mode
    sendcmd(control_sockfd, "PASV", "227", recvline);

    dataport = getport(recvline);

    data_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in data_sock;
    data_sock.sin_family = AF_INET;

    bzero(&data_sock, sizeof(data_sock));
    //获取hostent中相关参数
    if (inet_pton(AF_INET, addr, &data_sock.sin_addr) == 0) {
        printf("Server IP is a error!\n");
        exit(1);
    }

    printf("%s:%d\n", addr, dataport);

    data_sock.sin_port = htons(dataport);
    if (connect(data_sockfd, (struct sockaddr *) &data_sock, sizeof(struct sockaddr)) == -1) {
        printf("pasv data connect is error!\n");
        exit(1);
    }
    // struct timeval tv;
    // tv.tv_sec = 300;  /* 30 Secs Timeout */
    // tv.tv_usec = 0;  // Not init'ing this can cause strange errors
    // setsockopt(data_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,sizeof(struct timeval));

    // bzero(recvline, sizeof(recvline));
    
    // if (recv(data_sockfd, recvline, sizeof(recvline), 0) < 0) {
    //     printf("Recv connect error\n");
    //     exit(1);
    // }

    sendcmd(control_sockfd, "STOR ./SPEEDTEST\r\n", "150", recvline);

    clock_t start = clock(), costed = 0;
    int chunksize = 65535, count = 0;

    while (costed < seconds)//start to transport the file!
    {
        char buffer[chunksize];
        bzero(buffer, sizeof(buffer));
        send(data_sockfd, buffer, chunksize, 0);
        count += 1;
        costed = clock() - start;
    }

    printf("bytes: %d, seconds: %f\n", count * chunksize, (double)costed / CLOCKS_PER_SEC);
    close(data_sockfd);

    return costed;
}


int main(int argc, char const *argv[]) {
    ftpFd fd = login("114.215.66.90", 21, "share", "share");
    if (fd > 0) {
        int s = speed(fd, "114.215.66.90", 50000); //50M
        printf("Speed is %d\n", s);
        logout(fd);
    } else {
        /* code */
    }
    return 0;
}
