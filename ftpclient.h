#ifndef _FTPCLIENT_
#define _FTPCLIENT_ "ftpclient.h"

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


#endif
