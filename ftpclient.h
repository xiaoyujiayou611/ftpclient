#ifndef __FTPCLIENT_H__
#define __FTPCLIENT_H__ "ftpclient.h"

typedef int ftpFd;

ftpFd login(char *server, int port, char * user, char * pwd);

void logout(ftpFd fd);

int speed_test(int bytes);

int speed_test_by_file(char * filename);

void ftp_list(ftpFd fd);

void ftp_pwd(ftpFd fd);

void ftp_changdir(char dir[], ftpFd fd);

void ftp_quit(ftpFd fd);

void ftp_creat_mkd(char *path, ftpFd fd);

int ftp_up(ftpFd fd);

void ftp_stru(ftpFd fd);

void ftp_rest(ftpFd fd);

char *itoa(int value, char *string, int radix);


#endif
