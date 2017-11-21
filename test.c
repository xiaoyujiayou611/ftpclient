#include "ftpclient.h"
#include "stdio.h"


int main(int argc, char const *argv[]) {
    double s = speed_test("114.215.66.90", 21, "share", "admin", 10);
    printf("Speed: %f Bytes/s, %f KB/s, %f MB/s\n", s, s/1024, s/1024/1024);
}
