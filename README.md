# FTP 测试客户端

# 使用方式

```c
#include "ftpclient.h"
#include "stdio.h"


int main(int argc, char const *argv[]) {
    struct Speed s = speed_test("114.215.66.90", 21, "share", "admin", 10);
    printf("Speed upload: %f Bytes/s, %f KB/s, %f MB/s\n", s.upload, s.upload/1024, s.upload/1024/1024);
    printf("Speed download: %f Bytes/s, %f KB/s, %f MB/s\n", s.download, s.download/1024, s.download/1024/1024);
}

```

样例输出:

```
Speed upload: 1206560.943906 Bytes/s, 1178.282172 KB/s, 1.150666 MB/s
Speed download: 1254346.777547 Bytes/s, 1224.948025 KB/s, 1.196238 MB/s
```

# 机制

本程序使用socket实现了FTP通信协议, 进而对指定网卡进行通信速率测试.

## socket通信

Socket 客户端编程主要步骤如下：

1. socket() 创建一个 Socket
2. connect() 与服务器连接
3. write() 和 read() 进行会话
4. close() 关闭 Socket

Socket 服务器端编程主要步骤如下：

1. socket() 创建一个 Socket
2. bind()
3. listen() 监听
4. accept() 接收连接的请求
5. write() 和 read() 进行会话
6. close() 关闭 Socket

## FTP协议

主要命令介绍：

1. USER:  指定用户名. 通常是控制连接后第一个发出的命令. "USER admin\r\n": 用户名为`admin`登录.
2. PASS:  指定用户密码. 该命令紧跟USER命令后. "PASS password\r\n": 密码为 `password`.
3. SIZE:  从服务器上返回指定文件的大小. "SIZE file.txt\r\n": 如果file.txt文件存在, 则返回该文件的大小.
4. EPASV: 让服务器在数据端口监听, 进入被动模式. 如: "EPASV\r\n".
5. RETR:  下载文件. "RETR file.txt\r\n": 下载文件 file.txt.
6. STOR:  上传文件. "STOR file.txt\r\n": 上传文件 file.txt.

主要流程:

1. 客户端和 FTP 服务器建立 Socket 连接.
2. 向服务器发送 USER, PASS 命令登录 FTP 服务器.
3. 使用 EPASV 命令得到服务器监听的端口号, 建立数据连接.
4. 使用 RETR/STOR 命令下载/上传文件.


### 文件测速流程

1. 建立网络连接
2. 发送登录命令

    USER share: 331 Please specify the password.
    PASS admin: 230 Login successful.

3. 切换到二进制模式

    TYPE I: 200 Switching to Binary mode.

4. 请求服务器打开EPSV模式

    EPSV: 229 Entering Extended Passive Mode (|||60038|).

5. 解析得到EPSV端口, 新建到EPSV端口的网络连接

6. 发送上传命令, 统计上传数据量和时间, 计算出上传速率, 关闭数据端口

    STOR SPEEDTEST: 150 Ok to send data.

7. 发送SIZE命令, 获取测试文件的字节数

    SIZE SPEEDTEST: 213 12066816.

8. 请求打开新的EPSV端口用于数据下载

    EPSV: 229 Entering Extended Passive Mode (|||36931|).

9. 发送下载命令, 同时连接到EPSV端口, 读取字节流, 并记录读取时间, 计算出下载速率

    RETR SPEEDTEST: 150 Opening BINARY mode data connection for SPEEDTEST (12066816 bytes).

## 测试模型

由于需要测试多网卡系统, 因此对于不同的网卡建立不通的线程, 主进程分别启动多个测速线程, 进行多网卡测试.
