Sget (Simple get)
=================

Request a line from user while providing vi like line editing tools

## Usage

[main.c](./main.c)

```c
#include <stdio.h>

extern int sget(char*, int);

int main() {
    #define BUF 127
    char buf[BUF] = {0};

    sget(buf, BUF);

    printf("%s\n", buf);

    return 0;
}
```

### Build

```sh
cc -o main main.c sgets.c
```

## Info

```
normal mode: '] '
  i | \n        insert mode '> '
  a             insert mode and move to end
  4 | $         move to end
  6 | 0         move to beginning
  h             move to left
  l             move to right
  w | ctrl+h    move a word to left
  e | ctrl+l    move a word to right
  x             delete character at cursor
insert mode: '> '
  ctrl+[ | esc  normal mode '] '
  <space> | a-z insert character
  <backspace>   delete character
  \n            end input
... press any key to continue ...
```
