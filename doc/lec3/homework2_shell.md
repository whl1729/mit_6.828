# 《MIT 6.828 Homework 2: Shell》解题报告

## sh.c源码剖析

### 基础知识

1. isatty:  test whether a file descriptor refers to a terminal. tty stands for TeleTYpewriter.
    * HEADER FILE: `#include <unistd.h>`
    * SYNOPSIS: `int isatty(int fd);`
    * DESCRIPTION: The isatty() function tests whether fd is an open file descriptor referring to a terminal.
    * RETURN VALUE: isatty() returns 1 if fd is an open file descriptor referring to a terminal; otherwise 0 is returned, and errno is set to indicate the error.
    * ERRORS: EBADF -- fd is not a valid file descriptor. EINVAL -- fd refers to a file other than a terminal. POSIX.1 specifies the error ENOTTY for this case.

2. The function fileno() examines the argument  stream  and  returns its integer file descriptor.

3. stdin: Originally I/O happened via a physically connected system console (input via keyboard, output via monitor), but standard streams abstract this. When a command is executed via an interactive shell, the streams are typically connected to the text terminal on which the shell is running, but can be changed with redirection or a pipeline. More generally, a child process will inherit the standard streams of its parent process.
