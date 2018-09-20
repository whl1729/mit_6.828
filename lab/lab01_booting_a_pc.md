# 《MIT 6.828 Lab1: Booting a PC》实验报告

## 实验内容

1. 熟悉x86汇编语言、QEMU x86仿真器、PC开机引导流程
2. 测试6.828 内核的启动加载器（boot loader）
3. 研究6.828 内核的初始化模板（JOS）

## 实验题目

注意：部分Exercise的解答过程较长，因此专门新建一个文档来记录解答过程，而在本文中给出其链接。

### Exercise 1：阅读汇编语言资料

> Exercise 1. Familiarize yourself with the assembly language materials available on the [6.828 reference page](https://pdos.csail.mit.edu/6.828/2017/reference.html). You don't have to read them now, but you'll almost certainly want to refer to some of this material when reading and writing x86 assembly.

> We do recommend reading the section "The Syntax" in [Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html). It gives a good (and quite brief) description of the AT&T assembly syntax we'll be using with the GNU assembler in JOS.

#### 解答

1. 读完《PC Assembly Language》，并输出[学习笔记](read_pc_assembly_language.md)。
2. 读完《Brennan's Guide to Inline Assembly》，并输出[学习笔记](read_brennans_guide_to_inline_assembly.md)。

#### 备注

1. [PC Assembly Language Book](https://pdos.csail.mit.edu/6.828/2017/readings/pcasm-book.pdf)是一本学习x86汇编语言的好书，不过要注意此书的例子是为NASM汇编器而设计，而我们课程使用的是GNU汇编器。

2. NASM汇编器使用Intel语法，而GNU汇编器使用AT&T语法。两者的语法差异可以参考[Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html)。

### Exercise 2：使用GDB命令跟踪BIOS做了哪些事情

见[《MIT 6.828 Lab 1 Exercise 2》实验报告](lab01_exercise02_trace_into_bios.md)。

### Exercise 3: 使用GDB命令跟踪boot loader做了哪些事情

见[《MIT 6.828 Lab 1 Exercise 3》实验报告](lab01_exercise03_trace_into_boot_loader.md)。

## 实验笔记

### 环境部署

#### 安装编译工具链
参考[Tools Used in 6.828: Compiler Toolchain](https://pdos.csail.mit.edu/6.828/2017/tools.html##chain)。根据`objdump -i`和`gcc -m32 -print-libgcc-file-name`命令的输出结果，可以确认我的Ubuntu环境已经支持6.828所需的工具链，因此跳过这一环节。

#### 安装QEMU仿真器
参考[Tools Used in 6.828: QEMU Emulator](https://pdos.csail.mit.edu/6.828/2017/tools.html#qemu)以及[Xin Qiu: MIT 6.828 Lab 1](https://xinqiu.me/2016/10/15/MIT-6.828-1/)。

### Part 1: PC Bootstrap

#### 模拟x86

1. make命令
    * `make`：编译最小的6.828启动加载器和内核
    * `make qemu`：运行QEMU。控制台输出会同时打印在QEMU虚拟VGA显示和虚拟PC的虚拟串口
    * `make qemu-nox`：运行QEMU。控制台输出只会打印在虚拟串口

#### PC物理地址空间

1. 早期基于8088处理器的PC只支持1MB的物理地址寻址
    * 0x00000000 ~ 0x000A0000：640KB，Low Memory，早期PC能够使用的RAM地址。
    * 0x000A0000 ~ 0x000FFFFF：384KB，预留给硬件使用，比如视频显示缓存、存储固件等。
        * 0x000A0000 ~ 0x000C0000: 128KB，VGA显示
        * 0x000C0000 ~ 0x000F0000: 192KB，16-bit devices, expansion ROMs
        * 0x000F0000 ~ 0x00100000: 64KB，BIOS RAM

2. 后来80286和80386处理器出现，能够支持16MB乃至4GB的物理地址空间，但仍然预留最低的1MB物理地址空间，以便后向兼容已有软件。因此现代PC在0x000A0000和0x00100000这段内存空间中存在hole，把RAM划分成“low memory”（或“conventional memory”，最低的640KB内存）和“extend memory”（其他内存）两部分。

### Part 2: The Boot loader

1. 当BIOS发现一个可启动的硬盘时，会将512字节的启动扇区加载到地址为0x7c00到0x7dff的内存中，然后使用jmp指令将CS:IP设置为0000:7c00，从而将控制权交给boot loader。

2. 6.828的boot loader由`boot/boot.S`和`boot/main.c`两个文件组成。

3. boot loader主要完成两个任务：
    * 将处理器由实模式切换到虚模式。
    * 从硬盘中读取内核（通过直接访问IDE磁盘设备寄存器）

## 问题汇总

1. Q：`make qemu`进入QEMU界面后如何退出？目前我只能通过关闭终端来退出。

2. Q：`make qemu-gdb`进入QEMU界面，然后通过关闭终端退出，再次`make qemu-gdb`时报错：“qemu-system-i386: -gdb tcp::25000: Failed to bind socket: Address already in use”，怎么解决？
   A: 发生这种问题是由于端口被程序绑定而没有释放造成。可以使用`netstat -lp`命令查询当前处于连接的程序以及对应的进程信息。然后用`ps pid`察看对应的进程，并使用`kill pid`关闭该进程即可。
