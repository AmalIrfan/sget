#!/usr/bin/env python3
import os
import pty
import subprocess
import time

def run_with_pty(input):
    pid, fd = pty.fork()

    if pid == 0:
        os.execlp("./main", "./main")
        os.exit(1)

    output = os.read(fd, 1024);
    for ch in input:
        os.write(fd, bytes((ch,)))
        time.sleep(0.02)
    output += os.read(fd, 1024)

    os.waitid(os.P_ALL, 0, os.WEXITED)
    return output

import pickle

tests = dict.fromkeys([
    b"Hello\x1b\x0a\x7fi\x0a",
    b"Helloooo World\x1b\x08\n\x7f\x7f\x7f\n",
    b"    Hello World\x1b\x08\x08\x08xxxx\n\n",
])

if os.path.exists("dump.bin"):
    with open("dump.bin", "rb") as fh:
        tests.update(pickle.load(fh))

def diff(r, v):
    r = repr(r)
    v = repr(v)
    print(r)
    print(v)
    for i, j in zip(r, v):
        if (i == j):
            print(end=" ")
        else:
            print(end="*")
    print()

for t, v in tests.items():
    r = run_with_pty(t);
    if v:
        print("test", repr(t))
        assert r == v, diff(r, v)
    tests[t] = r

with open("dump.bin", "wb") as fh:
    pickle.dump(tests, fh)
