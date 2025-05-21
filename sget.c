#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>
#include <poll.h>

#define SGET_HELP \
    "\r\n"\
    "normal mode: '] '\r\n" \
    "  i | \\n        insert mode '> '\r\n" \
    "  a             insert mode and move to end\r\n" \
    "  4 | $         move to end\r\n" \
    "  6 | 0         move to beginning\r\n" \
    "  h             move to left\r\n" \
    "  l             move to right\r\n" \
    "  w | ctrl+h    move a word to left\r\n" \
    "  e | ctrl+l    move a word to right\r\n" \
    "  x             delete character at cursor\r\n" \
    "insert mode: '> '\r\n" \
    "  ctrl+[ | esc  normal mode '] '\r\n" \
    "  <space> | a-z insert character\r\n" \
    "  <backspace>   delete character\r\n" \
    "  \\n            end input\r\n" \
    "... press any key to continue ..."

#define SGET_LINESHELP 16

static char* _sget_numstr(int n) {
    static char buf[10];
    static char sbuf[9];
    int i = 0;
    int j = 0;
    while (n && i < 9) {
        sbuf[i++] = n % 10;
        n /= 10;
    }
    while (j < i) {
        buf[j] = '0' + sbuf[i - j - 1];
        j++;
    }
    if (!j) {
        buf[j++] = '0';
    }
    buf[j] = 0;
    return buf;
}

int sget(char *buf, int size) {
    struct termios old_termios = {0};
    struct termios new_termios = {0};
    int fd = 0;
    char ch = 0;
    int n = 0;
    static char tmp[30] = {0};
    int cmd = 0;
    int col = 0;

    if (size > 256) {
        /* Unchecked hardcoded limits: _sget_numstr: sget: tmp; buf and sbuf; */
        fprintf(stderr, "Hardcoded limitter: sget: size is greater than 256\n");
        exit(1);
    }

    fd = open("/dev/tty", O_RDWR);
    tcgetattr(fd, &old_termios);
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fd, TCSANOW, &new_termios);

    write(fd, "\r> ", 3);
    while (n < size - 1) {
        read(fd, &ch, 1);
        if (cmd) {
            ch = toupper(ch);
            if (ch == 'A') { /* insert mode append */
                cmd = 0;
                col = n;
                if (n) {
                    strcat(tmp, "\r> \033[");
                    strcat(tmp, _sget_numstr(n));
                    strcat(tmp, "C");
                    write(fd, tmp, strlen(tmp));
                    tmp[0] = 0;
                }
                else {
                    write(fd, "\r> ", 3);
                }
            }
            else if (ch == 'I' || ch == 10) { /* insert mode */
                cmd = 0;
                if (n && col) {
                    strcpy(tmp, "\r> \033[");
                    strcat(tmp, _sget_numstr(col));
                    strcat(tmp, "C");
                    write(fd, tmp, strlen(tmp));
                    tmp[0] = 0;
                }
                else {
                    write(fd, "\r> ", 3);
                }
            }
            else if (ch == 'H') { /* move left */
                if (col) {
                    write(fd, "\033[D", 3);
                    col--;
                }
            }
            else if (ch == 'L') { /* move right */
                if (col < n - 1) {
                    write(fd, "\033[C", 3);
                    col++;
                }
            }
            else if (ch == '0' || ch == '6' || ch == '^') { /* move to begin */
                if (n && col) {
                    write(fd, "\r\033[2C", 5);
                    col = 0;
                }
            }
            else if (ch == '4' || ch == '$') { /* move to end */
                if (col < n) {
                    strcpy(tmp, "\033[");
                    strcat(tmp, _sget_numstr(n - col));
                    strcat(tmp, "C");
                    write(fd, tmp, strlen(tmp));
                    col = n;
                }
            }
            else if (ch == 'B' || ch == 8) { /* CTRL+H */
                if (n) {
                    int cur = col;
                    if (buf[col - 1] == ' ')
                        while (col && buf[--col] == ' ');
                    if (buf[col - 1] != ' ')
                        while (col && buf[--col] != ' ');
                    if (cur - col) {
                        strcpy(tmp, "\033[");
                        strcat(tmp, _sget_numstr(cur - col));
                        strcat(tmp, "D");
                        write(fd, tmp, strlen(tmp));
                    }
                }
            }
            else if (ch == 'W' || ch == 12) { /* CTRL+L */
                if (n) {
                    int cur = col;
                    if (buf[col - 1] == ' ')
                        while (col < n - 1 && buf[col - 1] == ' ')
                            col++;
                    if (buf[col - 1] != ' ')
                        while (col < n - 1 && buf[col - 1] != ' ')
                            col++;
                    if (col - cur) {
                        strcpy(tmp, "\033[");
                        strcat(tmp, _sget_numstr(col - cur));
                        strcat(tmp, "C");
                        write(fd, tmp, strlen(tmp));
                    }
                }
            }
            else if (ch == 'X') { /* delete */
                if (col < n - 1) {
                    strcpy(tmp, buf + col + 1);
                    strcpy(buf + col, tmp);
                    n--;
                    tmp[0] = 0;
                    /* the delete only works if its seperate for some reason */
                    write(fd, "\033[K", 3);
                    strcpy(tmp, buf + col);
                    strcat(tmp, "\033[");
                    strcat(tmp, _sget_numstr(n - col));
                    strcat(tmp, "D");
                    write(fd, tmp, strlen(tmp));
                    tmp[0] = 0;
                }
                else if  (n > 0) {
                    write(fd, "\033[K", 3);
                    buf[--n] = 0;
                    if (col) {
                        col--;
                        write(fd, "\033[D", 3);
                    }
                }
            }
            else { /* print help and wait */
                if (ch == 27) {
                    /* ignore escape sequences */
                    struct pollfd pfd = { fd, POLLIN, 0 };
                    if (poll(&pfd, 1, 10)) {
                        read(fd, tmp, 2);
                        tmp[0] = 0;
                    }
                }

                write(fd, SGET_HELP, sizeof(SGET_HELP));
                read(fd, &ch, 1);
                for (int i = 0; i < SGET_LINESHELP; i++)
                    write(fd, "\033[2K\033[A", 7);
                if (col) {
                    strcpy(tmp, "\r] \033[");
                    strcat(tmp, _sget_numstr(col));
                    strcat(tmp, "C");
                    write(fd, tmp, strlen(tmp));
                }
                else
                    write(fd, "\r] ", 3);
                tmp[0] = 0;
            }
        }
        else {
            if (ch == 10 || ch == 04) /* end */
                break;
            else if (ch == 27) { /* enter cmd mode */
                /* ignore escape sequences */
                struct pollfd pfd = { fd, POLLIN, 0 };
                if (poll(&pfd, 1, 10)) {
                    read(fd, tmp, 2);
                    tmp[0] = 0;
                }

                cmd = 1;
                if (n) {
                    if (col == n)
                        col--;
                    strcpy(tmp, "\r] \033[");
                    strcat(tmp, _sget_numstr(col));
                    strcat(tmp, "C");
                    write(fd, tmp, strlen(tmp));
                    tmp[0] = 0;
                }
                else {
                    write(fd, "\r] ", 3);
                }
            }
            else if (ch < ' ') { /* print help and wait */
                write(fd, SGET_HELP, sizeof(SGET_HELP));
                read(fd, &ch, 1);
                for (int i = 0; i < SGET_LINESHELP; i++)
                    write(fd, "\033[2K\033[A", 7);

                if (col) {
                    strcpy(tmp, "\r> \033[");
                    strcat(tmp, _sget_numstr(col));
                    strcat(tmp, "C");
                    write(fd, tmp, strlen(tmp));
                    tmp[0] = 0;
                }
                else
                    write(fd, "\r> ", 3);
            }
            else if (ch == 127) { /* backspace */
                if (col < n) {
                    if (col) {
                        strcpy(tmp, buf + col);
                        col--;
                        strcpy(buf + col, tmp);
                        n--;
                        tmp[0] = 0;
                        /* the delete only works if its seperate for some reason */
                        write(fd, "\033[D\033[K", 6);
                        strcpy(tmp, buf + col);
                        strcat(tmp, "\033[");
                        strcat(tmp, _sget_numstr(n - col));
                        strcat(tmp, "D");
                        write(fd, tmp, strlen(tmp));
                        tmp[0] = 0;
                    }
                }
                else if (n > 0) {
                    buf[--n] = 0;
                    col--;
                    write(fd, "\033[D\033[K", 6);
                }
            }
            else { /* insert character */
                if (col < n) {
                    strcpy(tmp, buf + col);
                    strcpy(buf + col + 1, tmp);
                    n++;
                    buf[col++] = ch;
                    strcpy(tmp, buf + col - 1);
                    strcat(tmp, "\033[");
                    strcat(tmp, _sget_numstr(n - col));
                    strcat(tmp, "D");
                    write(fd, tmp, strlen(tmp));
                    tmp[0] = 0;
                }
                else {
                    write(fd, &ch, 1);
                    buf[n++] = ch;
                    col++;
                }
            }
        }
    }
    buf[n++] = 0;
    write(fd, "\r\033[K", 4);

    tcsetattr(fd, TCSANOW, &old_termios);
    close(fd);

    return 0;
}
