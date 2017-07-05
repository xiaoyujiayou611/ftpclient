#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define SERV_PORT   21
#define BUFFERSIZE  1024
int login_yes = 0;
static int control_sockfd;
int npsupport;
int f;

#define LOGIN_SUCCESS   1
#define LOGIN_FAILED    2
#define FTP_USER        "ftpuser"
#define FTP_PASS        "admin"

int login();

void ftp_list(int control_sockfd);

void ftp_pwd(int control_sockfd);

void ftp_changdir(char dir[], int control_sockfd);

void ftp_quit(int control_sockfd);

void ftp_creat_mkd(char *path, int control_sockfd);

int ftp_up(int control_sockfd);

void ftp_stru(int control_sockfd);

void ftp_rest(int control_sockfd);

char *itoa(int value, char *string, int radix);

int main(int argc, char **argv) {
    char command[BUFFERSIZE];
    char *cmd;

    int try_count = 0;
    while (try_count < 3) {
        if (login() == LOGIN_SUCCESS) {
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

int login() {
    //初始化端口信息
    char senddate, recvdate;
    char sendline[BUFFERSIZE], recvline[BUFFERSIZE];
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);

    //获取hostent中相关参数
    char name[BUFFERSIZE], password[BUFFERSIZE];
    printf("Login Server: ", name);
    scanf("%s", name);
    if (inet_pton(AF_INET, name, &serv_addr.sin_addr) == 0) {
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
        printf("connect success,pelase enter username\n");
    } else {
        printf("220 connect is error!");
        return LOGIN_FAILED;
    }
    //ftp用户登录主体部分
    int sendbytes, recvbytes;
    bzero(name, BUFFERSIZE);
    bzero(password, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    bzero(sendline, BUFFERSIZE);
    printf("ftp-> ");
    sprintf(sendline, "USER ftpuser\r\n");
    printf("--->%s\n", sendline);
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes == -1) {
        printf("send is wrong\n");
        login_yes = 0;
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (strncmp(recvline, "331", 3) == 0) {
        printf("331 please specify the password.\n");
    } else {
        printf("recv date is error.\n");
        login_yes = 0;
    }
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    printf("ftp-> ");
    sprintf(sendline, "PASS admin\r\n");
    printf("--->%s\n", sendline);
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes == -1) {
        printf("pass send is error\n");
        login_yes = 0;
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (strncmp(recvline, "230", 3) == 0) {
        printf("login success!\n");
        login_yes = 1;
    } else {
        printf("pass recv is error\n");
        login_yes = 0;
    }    //支持断点续传
    bzero(sendline, BUFFERSIZE);
    bzero(recvline, BUFFERSIZE);
    sprintf(sendline, "REST %s\r\n", "0");
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes == -1) {
        printf("rest send is error!\n");
        login_yes = 0;
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (recvbytes == -1) {
        printf("rest recv date is error.\n");
        login_yes = 0;
    }
    if (strncmp(recvline, "350 Restart position accepted (0).", 34) == 0) {
        npsupport = 1;
        printf("support 断点续传\n");
        login_yes = 1;
    } else {
        npsupport = 0;
        printf("not support 断点续传\n");
        login_yes = 0;
    }
//获取服务器版本信息
    bzero(recvline, BUFFERSIZE);
    bzero(sendline, BUFFERSIZE);
    sprintf(sendline, "SYST\r\n");

    //  printf("--->%s\n",sendline);
    sendbytes = send(control_sockfd, sendline, strlen(sendline), 0);
    if (sendbytes == -1) {
        printf("syst send is error\n");
        login_yes = 0;
    }
    recvbytes = recv(control_sockfd, recvline, sizeof(recvline), 0);
    if (recvbytes == -1) {
        printf("syst recv is error\n");
        login_yes = 0;
    }
    if (strncmp(recvline, "215 UNIX Type: L8", 17) == 0) {
        printf("%s", recvline);
        login_yes = 1;
    } else {
        printf("syst recv connectin is error\n");
        login_yes = 0;
    }


    return login_yes;

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
        printf(" type send is error!\n");
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
