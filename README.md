# FTP 测试客户端

# 使用方式

```c
#include "ftpclient.h"

int main() {

}

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


### 文件上传流程

### 下载流程

## 测试模型
