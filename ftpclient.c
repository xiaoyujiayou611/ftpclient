#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include "ftpclient.h"

#define PBSTR "--------------------------------------------------"
#define PBWIDTH 50
#define SERV_PORT   21
#define BUFFERSIZE  1024
int login_yes = 0;
int npsupport;
int f;

#define LOGIN_FAILED    -1


void error(char *p) {
    printf("Error: %s\n", p);
}

void printProgress (double percentage)
{
    int val = (int) (percentage * 100);
    if (val > 100) {
        val = 100;
    }
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf ("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush (stdout);
}


double milliseconds() {
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ; // convert tv_sec & tv_usec to millisecond
}


int getport(char * str) {
    int i, j;
    char *ptr1, *ptr2;
    char num[BUFFERSIZE];
    bzero(num, BUFFERSIZE);
    //取低位字节
    ptr1 = str + strlen(str);
    while (*(ptr1) != '|') {
        ptr1--;
    }
    ptr2 = ptr1 - 1;
    while (*(ptr2) != '|')
        ptr2--;

    strncpy(num, ptr2 + 1, ptr1 - ptr2 - 1);
    return atoi(num);
    //取高位字节
    // bzero(num, BUFFERSIZE);
    // ptr1 = ptr2;
    // ptr2--;
    // while (*(ptr2) != ',')
    //     ptr2--;
    // strncpy(num, ptr2 + 1, ptr1 - ptr2 - 1);
    // j = atoi(num);
    // return j * 256 + i;
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
        strncpy(ret, recvline, sizeof(recvline));
    } else {
        printf("Exec cmd %s error: %s\n", cmd, recvline);
        exit(1);
    }
}


struct Speed speed(ftpFd control_sockfd, char * addr, int seconds) {
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    int dataport, size;
    struct Speed speed;
    ftpFd data_sockfd;
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);

    // binary mode
    sendcmd(control_sockfd, "TYPE I", "200", recvline);
    

    //PASV mode
    sendcmd(control_sockfd, "EPSV", "229", recvline);

    dataport = getport(recvline);

    data_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in data_sock;
    bzero(&data_sock, sizeof(data_sock));
    data_sock.sin_family = AF_INET;

    //获取hostent中相关参数
    if (inet_pton(AF_INET, addr, &data_sock.sin_addr) == 0) {
        printf("Server IP is a error!\n");
        exit(1);
    }

    data_sock.sin_port = htons(dataport);
    int ret = connect(data_sockfd, (struct sockaddr *) &data_sock, sizeof(struct sockaddr));
    if ( ret == -1) {
        printf("Connect to data socket error: %s:%d, ret: %d, error: %s\n", addr, dataport, ret, strerror(errno));
        exit(1);
    }

    // upload speed
    sendcmd(control_sockfd, "STOR SPEEDTEST", "150", recvline);

    double start = milliseconds(), costed = 0;
    int chunksize = 1024, count = 0;

    while ((int)costed < seconds * 1000)//start to transport the file!
    {
        char buffer[chunksize];
        bzero(buffer, sizeof(buffer));
        send(data_sockfd, buffer, chunksize, 0);
        count += 1;
        costed = milliseconds() - start;
        //printf("Process: %f\n", (double)costed / (double)seconds / 1000);
        printProgress(costed / (double)seconds / 1000);
    }


    close(data_sockfd);
    sendcmd(control_sockfd, "", "226", recvline);

    printf("\nbytes: %d, seconds: %d\n\n", count * chunksize, (int)costed / 1000);
    speed.upload = (double)count * (double)chunksize / (double)costed * 1000;

    sendcmd(control_sockfd, "SIZE SPEEDTEST", "213", recvline);
    size = atoi(&recvline[4]);
    printf("Total transfer size: %d byte\n", size);

    // download speed
    // reopen data socket
    //PASV mode
    sendcmd(control_sockfd, "EPSV", "229", recvline);

    dataport = getport(recvline);

    data_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    data_sock.sin_family = AF_INET;

    bzero(&data_sock, sizeof(data_sock));
    //获取hostent中相关参数
    if (inet_pton(AF_INET, addr, &data_sock.sin_addr) == 0) {
        printf("Server IP is a error!\n");
        exit(1);
    }

    data_sock.sin_port = htons(dataport);
    ret = connect(data_sockfd, (struct sockaddr *) &data_sock, sizeof(struct sockaddr));
    if ( ret == -1) {
        printf("Connect to data socket error: %s:%d, ret: %d, error: %s\n", addr, dataport, ret, strerror(errno));
        exit(1);
    }

    sendcmd(control_sockfd, "RETR SPEEDTEST", "150", recvline);

    start = milliseconds(), costed = 0;
    count = 0, chunksize = 0;

    while ((chunksize = recv(data_sockfd, recvline, sizeof(recvline), 0)) > 0)//start to transport the file!
    {
        count += chunksize;
        costed = milliseconds() - start;
        //printf("Process: %f\n", (double)costed / (double)seconds / 1000);
        printProgress(count / (double)size);
    }

    printf("\nbytes: %d, seconds: %d\n\n", count, (int)costed / 1000);
    speed.download = (double)count / (double)costed * 1000;

    close(data_sockfd);

    return speed;
}


struct Speed speed_test(char * host, int port, char * username, char * password, int seconds) {
    ftpFd fd = login(host, port, username, password);
    struct Speed result;
    if (fd > 0) {
        result = speed(fd, host, seconds); //50M
        logout(fd);
        return result;
    } else {
        /* code */
    }
    return result;
}


int cli(int argc, char **argv) {
    char command[BUFFERSIZE];
    char *cmd;
    ftpFd control_sockfd;

    int try_count = 0;
    while (try_count < 3) {
        control_sockfd = login("127", 123, "", "");
        if ( control_sockfd != LOGIN_FAILED) {
            printf("Login Success!\n");
            break;
        }
        printf("Login Failed, left %d try times.\n", 2 - try_count);
        try_count ++;
    }
    if (try_count == 3) {
        exit(1);
    }
    while (1) {
        sleep(1);
        printf("ftp>");
        bzero(command, BUFFERSIZE);
        scanf("%s", command);
        cmd = command;
        while (*(cmd) == ' ')
            cmd++;
        if (strncmp(cmd, "pasv", 4) == 0) {
            ftp_list(control_sockfd);
        }
        if (strncmp(cmd, "port", 4) == 0) {
            ftp_list(control_sockfd);
        }
        if (strncmp(cmd, "pwd", 3) == 0) {
            ftp_pwd(control_sockfd);
        }
        if (strncmp(cmd, "mkdir", 5) == 0) {
            char path[60];
            bzero(path, 60);
            printf("创建的路径名: ");
            scanf("%s", path);
            ftp_creat_mkd(path, control_sockfd);
        }
        if (strncmp(cmd, "cd", 2) == 0) {
            int i;
            char path[60];
            bzero(path, 60);
            printf("要到的路径：");
            scanf("%s", path);
            printf("%s\n", path);
            ftp_changdir(path, control_sockfd);
        }
        if (strncmp(cmd, "up", 3) == 0) {
            ftp_pwd(control_sockfd);
            ftp_up(control_sockfd);
        }
        if (strncmp(cmd, "quit", 4) == 0) {
            printf("bye^_^\n");
            close(control_sockfd);
            break;
        }
        printf("支持 list,pwd,mkdir,back,cd,up\n");
    }

    return 0;

}

void logout(ftpFd fd) {
    close(fd);
}

ftpFd login(char * server, int port, char * user, char * pwd) {
    //初始化端口信息
    char senddate, recvdate;
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    int control_sockfd;
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    //获取hostent中相关参数
    char name[BUFFERSIZE], password[BUFFERSIZE];
    printf("Connect to server: %s:%d\n", server, port);
    if (inet_pton(AF_INET, server, &serv_addr.sin_addr) == 0) {
        printf("Server IP is a error!\n");
        return LOGIN_FAILED;
    }

    //创建socket
    control_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (control_sockfd < 0) {
        printf("socket is error\n");
        return LOGIN_FAILED;
    }

    if ((connect(control_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        printf("connect is error\n");
        return LOGIN_FAILED;
    }
    recvdate = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (recvdate == -1) {
        printf("recvdate is connect error\n");
        return LOGIN_FAILED;
    } else if (strncmp(recvline, "220", 3) == 0) {
        printf("%s\n", recvline);
    } else {
        printf("220 connect is error!");
        return LOGIN_FAILED;
    }
    
    //ftp用户登录主体部分
    int sendbytes, recvbytes;
    bzero(recvline, BUFFERSIZE);
    bzero(sendline, BUFFERSIZE);

    //发送登录命令
    snprintf(sendline, sizeof(sendline), "USER %s", user);
    sendcmd(control_sockfd, sendline, "331", recvline);

    //发送密码
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    snprintf(sendline, sizeof(sendline), "PASS %s", pwd);
    sendcmd(control_sockfd, sendline, "230", recvline);

    //是否支持断点续传
    bzero(recvline, BUFFERSIZE);
    sendcmd(control_sockfd, "REST 0", "350", recvline);

    //获取服务器版本信息
    bzero(recvline, BUFFERSIZE);
    sendcmd(control_sockfd, "SYST", "215", recvline);

    return control_sockfd;
}

void ftp_quit(int control_sockfd) {
    char sendline[BUFFERSIZE];
    char recvline[BUFFERSIZE];
    int recvbytes;
    int sendbytes;
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    sprintf(sendline, "QUIT\r\n");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("quit send is error!\n");
        exit(1);
    }
    recvbytes = recv(control_sockfd, recvline, strlen(recvline), 0);
    if (strncmp(recvline, "221", 3) == 0) {
        printf("221 bye!^_^");
        exit(1);
    } else {
        printf("quit recv is error!\n");
        exit(1);
    }
}

void ftp_creat_mkd(char *path, int control_sockfd) {
    char sendline[BUFFERSIZE];
    char recvline[BUFFERSIZE];
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    int recvbytes, sendbytes;
    int issuccess;
    sprintf(sendline, "MKD %s\r\n", path);
    printf("%s\n", sendline);
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("mkd send is error!");
        exit(1);
    }
    recvbytes = recv(control_sockfd, recvline, strlen(recvline), 0);
    if (strncmp(recvline, "257", 3) == 0) {
        issuccess = 1;
    } else {
        issuccess = 0;
    }
}

void ftp_changdir(char *dir, int control_sockfd) {

    char sendline[BUFFERSIZE];
    char recvline[BUFFERSIZE];
    int recvbytes, sendbytes;
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    sprintf(sendline, "CWD %s\r\n", dir);
    printf("%s\n", sendline);
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("cwd send is error!\n");
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (recvbytes < 0) {
        printf("cwd recv is error!\n");
    }
    if (strncmp(recvline, "250", 3) == 0) {
        char buf[55];
        sprintf(buf, ">>> %s\n", recvline);
        printf("%s\n", buf);
    } else {
        printf("cwd chdir is error!\n");
        exit(1);
    }
}

//pwd 命令函数
//在应答中返回当前工作目录，“pwd”+\r\n
void ftp_pwd(int control_sockfd) {
    int recvbytes, sendbytes;
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    sprintf(sendline, "PWD\r\n");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("pwd,send is error\n");
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (strncmp(recvline, "257", 3) == 0) {
        int i = 0;
        char *ptr;
        char currendir[BUFFERSIZE];
        bzero(currendir, BUFFERSIZE);
        ptr = recvline + 5;
        while (*(ptr) != '"') {
            currendir[i++] = *(ptr);
            ptr++;
        }
        currendir[i] = '\0';
        printf("current directory is:%s\n", currendir);

    } else {
        printf("pwd,recv is error!\n");
    }
}


int ftp_up(int control_sockfd) {

    int pasv_or_port;// 定义the ftp协议的两种不同工作mode
    int recvbytes, sendbytes;
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    struct sockaddr_in serv_addr;

    FILE *fd;
    int i, j;
    int data_sockfd;
    //type
    bzero(recvline, BUFFERSIZE);
    bzero(sendline, BUFFERSIZE);
    sprintf(sendline, "TYPE I\r\n");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("type send is error!\n");
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    printf("%s\n", recvline);
    if (strncmp(recvline, "200", 3) == 0) {
        printf("使用二进制传输数据\n");
    } else {
        printf("type recv is error!\n");
    }

    if (npsupport == 1) {
        //open the file
        int size;
        char localpathname[60];//预打开的文件路径字符串
        int flags;
        char pathname[60];
        unsigned int mode;
        //用户来选择pasv 或者是 port mode
        char selectdata_mode_tran[BUFFERSIZE];
        bzero(selectdata_mode_tran, BUFFERSIZE);
        bzero(sendline, BUFFERSIZE);
        bzero(recvline, BUFFERSIZE);
        pasv_or_port = 0;//(默认是pasv模式)
        //pasv mode
        if (pasv_or_port == 0) {
            strcat(sendline, "PASV");
            strcat(sendline, "\r\n");
            sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
            if (sendbytes < 0) {
                printf("pasv send is error!\n");
            }
            recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
            if (recvbytes < 0) {
                printf("pasv recv is error!\n");
            }
            if (strncmp(recvline, "227", 3) == 0) {
                char buf[55];
                snprintf(buf, 52, ">>> %s\n", recvline);
                printf("%s\n", buf);
            } else {
                printf("pasv recv is error!\n");
            }
            //处理ftp server 端口
            char *ptr1, *ptr2;
            char num[BUFFERSIZE];
            bzero(num, BUFFERSIZE);
            //取低位字节
            ptr1 = recvline + strlen(recvline);
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
            int data_serviceport;
            data_serviceport = j * 256 + i;
            data_sockfd = socket(AF_INET, SOCK_STREAM, 0);

            serv_addr.sin_family = AF_INET;
            char address[BUFFERSIZE];
            /*scanf("%s",address);
            if(inet_pton(AF_INET,address,&serv_addr.sin_addr)==0)
            {
                printf("Server IP is a error!\n");
                exit(1);
            }*/
            bzero(&serv_addr, sizeof(serv_addr));
            //获取hostent中相关参数
            char name[BUFFERSIZE];
            scanf("%s", name);
            if (inet_pton(AF_INET, name, &serv_addr.sin_addr) == 0) {
                printf("Server IP is a error!\n");
                exit(1);
            }
            //serv_addr.sin_addr.s_addr=htons("10.75.175.249");
            serv_addr.sin_port = htons(data_serviceport);
            if (connect(data_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) == -1) {
                printf("pasv data connect is error!\n");
            }
            printf("local-file-pathname=");
            scanf("%s", localpathname);
            printf("remote-file-pathname=");
            scanf("%s", pathname);
            printf("local:%s remote:%s\n", localpathname, pathname);
            fd = fopen(pathname, "r");
            if (fd == NULL) {
                printf("cannot open file,请重新输入！\n");
            }
            bzero(sendline, BUFFERSIZE);
            bzero(recvline, BUFFERSIZE);
            strcat(sendline, "STOR ");
            strcat(sendline, localpathname);
            strcat(sendline, "\r\n");
            printf("%s\n", sendline);
            sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
            if (sendbytes < 0) {
                printf("stor send is error!\n");
            }
            recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
            printf("%s\n", recvline);
            if (recvbytes < 0) {
                printf("retr recv is error!\n");
            }
            if (strncmp(recvline, "150", 3) == 0) {
                char buf[55];
                snprintf(buf, 25, ">>> %s\n", recvline);
                printf("%s\n", buf);
            }
            int sendedbytes = 0;
            fseek(fd, 0, SEEK_END);//将文件位置指针置于文件结尾
            int countbytes = ftell(fd);//得到当前位置与文件开始位置的字节偏移量。
            fseek(fd, 0, SEEK_SET);
            printf("countbytes:%d\n", countbytes);
            while (!feof(fd))//start to transport the file!
            {
                char buffer[65536];
                bzero(buffer, sizeof(buffer));
                int size;
                size = fread(buffer, 1, sizeof(buffer), fd);
                if (ferror(fd)) {
                    printf("read file data is error!\n");
                    break;
                } else {
                    bzero(sendline, BUFFERSIZE);
                    clock_t start, end;
                    double cost;
                    start = clock();
                    sendbytes = send(data_sockfd, buffer, size, 0);
                    end = clock();
                    cost = end - start;
                    printf("传输了 %d 个字节\n", sendbytes);
                    sendedbytes += sendbytes;
                    double speed = sendbytes / (cost / 1000);
                    printf("the speed is %.2lfkpbs/s\n", speed / 1024);
                    double process = (double) sendedbytes / countbytes * 100;
                    printf("the process is %.2lf%%\n", process);
                    if (process > 100) {
                        process = 100;
                        printf("the process is %.2lf%%\n", process);
                        break;
                    }

                }
            }
            close(data_sockfd);
            fclose(fd);


        }


        return 0;
    }
    return 0;
}

char *itoa(int value, char *string, int radix) {
    char tmp[33];
    char *tp = tmp;
    int i;
    unsigned v;
    int sign;
    char *sp;

    sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned) value;
    while (v || tp == tmp) {
        i = v % radix;
        v = v / radix;
        if (i < 10)
            *tp++ = i + '0';
        else
            *tp++ = i + 'a' - 10;
    }

    if (string == 0)
        string = (char *) malloc((tp - tmp) + sign + 1);
    sp = string;

    if (sign)
        *sp++ = '-';
    while (tp > tmp)
        *sp++ = *--tp;
    *sp = 0;
    return string;
}

void ftp_list(int control_sockfd) {
    int pasv_or_port;// 定义the ftp协议的两种不同工作mode
    int recvbytes, sendbytes;
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    struct sockaddr_in serv_addr;
    int i, j;
    int flag = 0;
    int data_sockfd;

    //用户来选择pasv 或者是 port mode(默认的是pasv模式)
    char selectdata_mode_tran[BUFFERSIZE];
    bzero(selectdata_mode_tran, BUFFERSIZE);
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);

    pasv_or_port = 0;
    if (pasv_or_port == 0) {
        sprintf(sendline, "PASV\r\n");
        sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
        if (sendbytes < 0) {
            printf("pasv send is error!\n");
        }
        recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
        if (recvbytes < 0) {
            printf("pasv recv is error!\n");
        }
        if (strncmp(recvline, "227", 3) == 0) {
            printf("%s\n", recvline);
        } else {
            printf("pasv recv is error!\n");
        }
        //处理ftp server 端口
        char *ptr1, *ptr2;
        char num[BUFFERSIZE];
        bzero(num, BUFFERSIZE);
        //取低位字节
        ptr1 = recvline + strlen(recvline);
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
        //初始化服务器数据连接时的端口信息
        int data_serviceport;
        data_serviceport = j * 256 + i;
        data_sockfd = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(data_serviceport);
        if (connect(data_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr)) == -1) {
            printf("pasv data connect is error!\n");
        }
    }
    //type
    bzero(recvline, BUFFERSIZE);
    bzero(sendline, BUFFERSIZE);
    sprintf(sendline, "TYPE I\r\n");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf(" type send is error!\n");
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (strncmp(recvline, "200", 3) == 0) {
        printf("使用二进制传输数据\n");
    } else {
        printf("type recv is error!\n");
    }


    //list
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    sprintf(sendline, "LIST\r\n");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("list send is error!\n");
    }
    recvdata:
    sleep(1);
    recvbytes = recv(data_sockfd, recvline, sizeof(recvline), 0);
    if (recvbytes < 0) {
        close(data_sockfd);
        goto ending;
    }
    printf("%s", recvline);
    if (flag == 0) {
        bzero(recvline, BUFFERSIZE);
        recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
        if (strncmp(recvline, "226", 3) != 0) {
            flag = 1;
            goto recvdata;
        }
    }
    ending:
    if (flag != 1) {
        bzero(recvline, BUFFERSIZE);
    }
    close(data_sockfd);
}

//stru命令的实现
void ftp_stru(int control_sockfd) {
    int recvbytes, sendbytes;
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    sprintf(sendline, "STRU F\r\n");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("stru send is error!\n");
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (recvbytes < 0) {
        printf("stru recv is error!\n");
    }
    if (strncmp(recvline, "200", 3) == 0) {
        f = 0;
    }

}

//断点函数的支持
void ftp_rest(int control_sockfd) {

    int recvbytes, sendbytes;
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    sprintf(sendline, "REST 500 \r\n");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes < 0) {
        printf("stru send is error!/n");
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (recvbytes < 0) {
        printf("stru recv is error!/n");
    }
    if (strncmp(recvline, "350", 3) == 0) {
        printf("%s\n", recvline);
    }
}
