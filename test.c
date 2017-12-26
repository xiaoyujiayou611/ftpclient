#include "ftpclient.h"
#include "stdio.h"


int main(int argc, char const *argv[]) {
    struct Speed s = speed_test("114.215.66.90", 21, "share", "admin", 10);
    printf("Speed upload: %f Bytes/s, %f KB/s, %f MB/s\n", s.upload, s.upload/1024, s.upload/1024/1024);
    printf("Speed download: %f Bytes/s, %f KB/s, %f MB/s\n", s.download, s.download/1024, s.download/1024/1024);
}
