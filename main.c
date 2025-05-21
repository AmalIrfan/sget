#include <stdio.h>

extern int sget(char*, int);

int main() {
    #define BUF 127
    char buf[BUF] = {0};

    sget(buf, BUF);

    printf("%s\n", buf);

    return 0;
}
