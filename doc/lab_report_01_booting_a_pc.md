# 《Lab1: Booting a PC》实验报告

## 实验内容

1. 熟悉x86汇编语言、QEMU x86仿真器、PC开机引导流程
2. 测试6.828 内核的启动加载器（boot loader）
3. 研究6.828 内核的初始化模板（JOS）

## 实验题目

### Exercise 1：阅读汇编语言资料

#### 问题

> Exercise 1. Familiarize yourself with the assembly language materials available on the [6.828 reference page](https://pdos.csail.mit.edu/6.828/2017/reference.html). You don't have to read them now, but you'll almost certainly want to refer to some of this material when reading and writing x86 assembly.

> We do recommend reading the section "The Syntax" in [Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html). It gives a good (and quite brief) description of the AT&T assembly syntax we'll be using with the GNU assembler in JOS.

#### 解答

1. 读完《PC Assembly Language》，并输出[学习笔记](read_pc_assembly_language.md)。
2. 读完《Brennan's Guide to Inline Assembly》，并输出[学习笔记](read_brennans_guide_to_inline_assembly.md)。

#### 备注

1. [PC Assembly Language Book](https://pdos.csail.mit.edu/6.828/2017/readings/pcasm-book.pdf)是一本学习x86汇编语言的好书，不过要注意此书的例子是为NASM汇编器而设计，而我们课程使用的是GNU汇编器。

2. NASM汇编器使用Intel语法，而GNU汇编器使用AT&T语法。两者的语法差异可以参考[Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html)。

### Exercise 2：使用GDB命令跟踪BIOS做了哪些事情

#### 问题

> Exercise 2. Use GDB's si (Step Instruction) command to trace into the ROM BIOS for a few more instructions, and try to guess what it might be doing. You might want to look at [Phil Storrs I/O Ports Description](http://web.archive.org/web/20040404164813/members.iweb.net.au/~pstorr/pcbook/book2/book2.htm), as well as other materials on the [6.828 reference materials page](https://pdos.csail.mit.edu/6.828/2017/reference.html). No need to figure out all the details - just the general idea of what the BIOS is doing first.

#### 解答

使用si命令得到的前22条汇编指令如下。虽然能看懂每条指令的字面意思，但看不懂具体实现的功能，后来参考[myk的6.828 Lab1](https://zhuanlan.zhihu.com/p/36926462)大致理解了基本功能：设置ss和esp寄存器的值，打开A20门（为了兼容老芯片而留下的历史包袱）、进入保护模式（需要设置cr0寄存器的PE标志）。
```
[f000:fff0]    0xffff0:	ljmp   $0xf000,$0xe05b
[f000:e05b]    0xfe05b:	cmpl   $0x0,%cs:0x6ac8
[f000:e062]    0xfe062:	jne    0xfd2e1
[f000:e066]    0xfe066:	xor    %dx,%dx
[f000:e068]    0xfe068:	mov    %dx,%ss
[f000:e06a]    0xfe06a:	mov    $0x7000,%esp
[f000:e070]    0xfe070:	mov    $0xf34c2,%edx
[f000:e076]    0xfe076:	jmp    0xfd15c
[f000:d15c]    0xfd15c:	mov    %eax,%ecx
[f000:d15f]    0xfd15f:	cli    
[f000:d160]    0xfd160:	cld    
[f000:d161]    0xfd161:	mov    $0x8f,%eax
[f000:d167]    0xfd167:	out    %al,$0x70
[f000:d169]    0xfd169:	in     $0x71,%al
[f000:d16b]    0xfd16b:	in     $0x92,%al
[f000:d16d]    0xfd16d:	or     $0x2,%al
[f000:d16f]    0xfd16f:	out    %al,$0x92
[f000:d171]    0xfd171:	lidtw  %cs:0x6ab8
[f000:d177]    0xfd177:	lgdtw  %cs:0x6a74
[f000:d17d]    0xfd17d:	mov    %cr0,%eax
[f000:d180]    0xfd180:	or     $0x1,%eax
[f000:d184]    0xfd184:	mov    %eax,%cr0
[f000:d187]    0xfd187:	ljmpl  $0x8,$0xfd18f
```

#### 备注

1. CS（CodeSegment）和IP（Instruction Pointer）寄存器一起用于确定下一条指令的地址。

2. CLI：Clear Interupt，禁止中断发生。STL：Set Interupt，允许中断发生。CLI和STI是用来屏蔽中断和恢复中断用的，如设置栈基址SS和偏移地址SP时，需要CLI，因为如果这两条指令被分开了，那么很有可能SS被修改了，但由于中断，而代码跳去其它地方执行了，SP还没来得及修改，就有可能出错。

3. CLD: Clear Director。STD：Set Director。在字行块传送时使用的，它们决定了块传送的方向。CLD使得传送方向从低地址到高地址，而STD则相反。

4. 汇编语言中，CPU对外设的操作通过专门的端口读写指令来完成，读端口用IN指令，写端口用OUT指令。

5. LIDT: 加载中断描述符。LGDT：加载全局描述符。

6. 第一条指令：`[f000:fff0] 0xffff0:    ljmp   $0xf000,$0xe05b`
    * PC开始运行时，CS = 0xf000，IP = 0xfff0，对应物理地址为0xffff0。（计算公式： physical address = 16 * segment + offset）
    * 第一条指令做了jmp操作，跳到物理地址为0xfe05b的位置。

7. 控制寄存器：控制寄存器（CR0～CR3）用于控制和确定处理器的操作模式以及当前执行任务的特性。CR0中含有控制处理器操作模式和状态的系统控制标志；CR1保留不用；CR2含有导致页错误的线性地址；CR3中含有页目录表物理内存基地址，因此该寄存器也被称为页目录基地址寄存器PDBR（Page-Directory Base address Register）。
    * CR0的4个位：扩展类型位ET、任务切换位TS、仿真位EM和数学存在位MP用于控制80x86浮点（数学）协处理器的操作。
    * CR0的位0是PE（Protection Enable）标志。当设置该位时即开启了保护模式；当复位时即进入实地址模式。这个标志仅开启段级保护，而并没有启用分页机制。若要启用分页机制，那么PE和PG标志都要置位。
    * CR0的位31是PG（Paging，分页）标志。当设置该位时即开启了分页机制；当复位时则禁止分页机制，此时所有线性地址等同于物理地址。在开启这个标志之前必须已经或者同时开启PE标志。即若要启用分页机制，那么PE和PG标志都要置位。

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
