# 《Lab1: Booting a PC》实验报告

## 实验内容

1. 熟悉x86汇编语言、QEMU x86仿真器、PC开机引导流程
2. 测试6.828 内核的启动加载器（boot loader）
3. 研究6.828 内核的初始化模板（JOS）

## 实验题目

### Exercise 1：阅读汇编语言资料
阅读材料包括：
1. [the 6.828 reference page](https://pdos.csail.mit.edu/6.828/2017/reference.html)给出的汇编语言资料
2. [Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html)

### Exercise 2：使用GDB命令跟踪BIOS做了哪些事情


## 环境部署

### 安装编译工具链
参考[Tools Used in 6.828: Compiler Toolchain](https://pdos.csail.mit.edu/6.828/2017/tools.html##chain)。根据`objdump -i`和`gcc -m32 -print-libgcc-file-name`命令的输出结果，可以确认我的Ubuntu环境已经支持6.828所需的工具链，因此跳过这一环节。

### 安装QEMU仿真器
参考[Tools Used in 6.828: QEMU Emulator](https://pdos.csail.mit.edu/6.828/2017/tools.html#qemu)以及[Xin Qiu: MIT 6.828 Lab 1](https://xinqiu.me/2016/10/15/MIT-6.828-1/)。

## Part 1: PC Bootstrap

### 熟悉x86 汇编语言

1. 一本学习x86汇编语言的好书：[PC Assembly Language Book](https://pdos.csail.mit.edu/6.828/2017/readings/pcasm-book.pdf)，不过要注意此书的例子是为NASM汇编器而设计，而我们课程使用的是GNU汇编器。

2. NASM汇编器使用Intel语法，而GNU汇编器使用AT&T语法。两者的语法差异可以参考[Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html)。

3. CS（CodeSegment）和IP（Instruction Pointer）寄存器一起用于确定下一条指令的地址。

4. CLI：Clear Interupt，禁止中断发生。STL：Set Interupt，允许中断发生。CLI和STI是用来屏蔽中断和恢复中断用的，如设置栈基址SS和偏移地址SP时，需要CLI，因为如果这两条指令被分开了，那么很有可能SS被修改了，但由于中断，而代码跳去其它地方执行了，SP还没来得及修改，就有可能出错。

5. CLD: Clear Director。STD：Set Director。在字行块传送时使用的，它们决定了块传送的方向。CLD使得传送方向从低地址到高地址，而STD则相反。

6. 汇编语言中，CPU对外设的操作通过专门的端口读写指令来完成，读端口用IN指令，写端口用OUT指令。

7. LIDT: 加载中断描述符。LGDT：加载全局描述符。

### 模拟x86

1. make命令
    * `make`：编译最小的6.828启动加载器和内核
    * `make qemu`：运行QEMU。控制台输出会同时打印在QEMU虚拟VGA显示和虚拟PC的虚拟串口
    * `make qemu-nox`：运行QEMU。控制台输出只会打印在虚拟串口

### PC物理地址空间

1. 早期基于8088处理器的PC只支持1MB的物理地址寻址
    * 0x00000000 ~ 0x000A0000：640KB，Low Memory，早期PC能够使用的RAM地址。
    * 0x000A0000 ~ 0x000FFFFF：384KB，预留给硬件使用，比如视频显示缓存、存储固件等。
        * 0x000A0000 ~ 0x000C0000: 128KB，VGA显示
        * 0x000C0000 ~ 0x000F0000: 192KB，16-bit devices, expansion ROMs
        * 0x000F0000 ~ 0x00100000: 64KB，BIOS RAM

2. 后来80286和80386处理器出现，能够支持16MB乃至4GB的物理地址空间，但仍然预留最低的1MB物理地址空间，以便后向兼容已有软件。因此现代PC在0x000A0000和0x00100000这段内存空间中存在hole，把RAM划分成“low memory”（或“conventional memory”，最低的640KB内存）和“extend memory”（其他内存）两部分。

### The ROM BIOS

1. 第一条指令：`[f000:fff0] 0xffff0:    ljmp   $0xf000,$0xe05b`
    * PC开始运行时，CS = 0xf000，IP = 0xfff0，对应物理地址为0xffff0。（计算公式： physical address = 16 * segment + offset）
    * 第一条指令做了jmp操作，跳到物理地址为0xfe05b的位置。


## 问题汇总

1. Q：`make qemu`进入QEMU界面后如何退出？目前我只能通过关闭终端来退出。

2. Q：`make qemu-gdb`进入QEMU界面，然后通过关闭终端退出，再次`make qemu-gdb`时报错：“qemu-system-i386: -gdb tcp::25000: Failed to bind socket: Address already in use”，怎么解决？
   A: 发生这种问题是由于端口被程序绑定而没有释放造成。可以使用`netstat -lp`命令查询当前处于连接的程序以及对应的进程信息。然后用`ps pid`察看对应的进程，并使用`kill pid`关闭该进程即可。
