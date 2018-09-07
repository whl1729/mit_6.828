# 《PC Assembly Language》读书笔记

## 前言

1. 8086处理器只支持实模式（real mode），不能满足安全、多任务等需求。

> Q：为什么实模式不安全、不支持多任务？为什么虚模式能解决这些问题？
> A: 以下是根据网上搜索结果及自己的理解做出的解答，有待斟酌。(1) 安全：实模式下用户可以访问任意的物理内存，可以修改系统程序或重要数据的内容，因而不安全。虚模式下用户能够访问的内存是由Descriptor Table中的信息决定的，其基地址是事先不确定的，而长度、权限均有限制，因此相比实模式更安全。(2) 多任务：多任务意味着CPU可以在不同任务之间切换，这要求不同任务之间的地址空间要相互隔离，否则从A任务切换到B任务时，B任务可能会修改A任务使用的内存数据。实模式不支持数据隔离，而虚模式支持。

2. 本书充分使用了两个开源软件：NASM汇编器和DJGPP C/C++编译器。

3. [书中源码下载地址](http://pacman128.github.io/pcasm/)

## 第1章 简介

### 1.1 数字系统

1. 内存单位
----------- | ---------
word        | 2 bytes
double word | 4 bytes
quad word   | 8 bytes
paragraph   | 16 bytes

2. ASCII使用一个字节来对字符编码，而Unicode使用两个字节。

### 1.2 计算机组成

1. 8086 16-bit寄存器
    * AX/BX/CX/DX这四个16-bit的通用寄存器可以分解成两个8-bit的寄存器
    * SI和DI：index registers，通常用于指针，也可用于大多数通用寄存器可以使用的场景，但它们不能分解成8-bit的寄存器。
    * BP和SP寄存器用于指向机器语言栈里面的数据。
    * CS/DS/SS/ES：段寄存器。CS：Code Segment. DS: Data Segment. SS: Stack Segment. ES: Extra Segment.
    * IP寄存器和CS寄存器一起作为指示CPU将要执行的下一条指令的地址。每当执行完一条指令，IP将会指向下一条指令的地址。
    * FLAGS寄存器存储上一条指令的执行结果的重要信息。

2. 80386 32-bit寄存器
    * 相比8086，通用寄存器扩展到32-bit。为了后向兼容，AX仍然代表16-bit寄存器，而用EAX来代表扩展的32-bit寄存器。
    * 段寄存器仍然是16-bit，此外增加了两个备用的段寄存器FS和GS。

3. 实模式
    * 实模式的物理地址计算公式：physical address = 16 * selector + offset
    * 为什么不直接存储物理地址，而将其分解成两部分？回答：8086的地址需要20-bit的数字来表示，而寄存器只有16-bit，因此需要用两个16-bit的数值来表示。
    * 那为什么不是将20-bit的地址拆分成最高的4-bit和剩下的16-bit，而是用这种有点费解的表示方式？ 回答：不知道...
    * 实模式的缺点1：一个selector最多只能reference 64KB内存，当代码大于64KB时，需要分段存储，在不同段之间跳转时，CS的值也需要改变。（CS的值存储在selector中）
    * 实模式的缺点2：同一个物理地址对应的段地址不唯一，可以有多种表示方式。

4. 16-bit 保护模式
80286处理器使用16-bit保护模式。
    * 实模式下selector的值代表物理内存的paragraph数目，保护模式下selector的值代表descriptor table的索引。
    * 16-bit 保护模式使用了虚拟内存的技术，其基本思想是：只保留程序正在使用的数据和代码在内存中，其他数据和代码临时存储在磁盘上。
    * descriptor table记录有每个段的信息：是否在内存中、内存地址、访问权限等。
    * 16-bit 保护模式的一大缺点是offset仍然是16-bit，导致segment大小仍然现在在64KB。

5. 32-bit 保护模式
80386处理器使用32-bit保护模式，它与80286使用的16-bit保护模式的主要区别是：
    * offset被扩展到32-bits，因此offset的大小增大到40亿，从而段大小增大到4GB。
    * 段可以划分成大小为4KB的页。虚拟内存系统基于页而非段来工作。

6. 中断
    * 每种中断都分配有一个数字，用于索引中断向量表，找到对应的中断handler来处理当前中断。
    * 外部中断是指由鼠标、键盘、定时器等外围设备触发的中断。
    * 内部中断是指有CPU内部触发的中断，内部中断可能来自运行error或中断指令。Error Interrupt也被称作trap，由中断指令触发的中断也被称为软件中断。

### 1.3 汇编语言

1. 汇编语言
    * 汇编器是一个将汇编语言转换为机器语言的程序。
    * 汇编语言与高级语言的差异之一：每条汇编语言语句直接代表一条机器指令，而每条高级语言语句可能需要转换成多条机器指令。
    * 汇编语言与高级语言的差异之二：不同类型的CPU使用的汇编语言不相同，而高级语言则可以相同。因此汇编语言的可移植性要低于高级语言。
    * 本书使用NASM汇编器（Netwide Assembler），更通用的汇编器是微软汇编器MASM（Microsoft's Assembler）和Borland汇编器TASM。

2. 操作数
操作数有4种类型：
    * register
    * memory
    * immediate
    * implied：没直接表示出来的数，比如自加操作中的1.

3. driectives
汇编语言也有类似C语言的预处理语句，其语句是以%开头。
    * `SIZE equ 100`
    * `%define SIZE 100`
    * `L1 db 0`: byte labeled L1 with initial value 0
    * `L2 dw 1000`: word labeled L2 with initial value 1000
    * `L7 resb 1`: 1 uninitialized byte labeled L7
    * letters for RESX and DX Directives:
        * B: byte
        * W: word
        * D: double word
        * Q: quad word
        * T: ten bytes
    * label可以用来指向代码中的数据，label相当于指针，在label前后加上中括号（[]）则表示对指针所指向的数据，类似C语言的星号。

4. 输入/输出：
    * 作者在`asm_io.inc`文件中封装了C语言的I/O函数，包括`print_int`, `print_char`, `print_string`, `print_nl`, `read_int`和`read_char`。
    * 疑问：1.3.6节说汇编语言可以使用C标准库的I/O函数，为什么底层语言可以调用高级语言的函数的？
    * 汇编语言文件包含：`%include "asm_io.inc"`
    * 函数调用：使用CALL指令。


5. 调试：通过打印问题现场中寄存器、内存、栈和数学处理器里面的数值来进行调试。作者在`asm_io.inc`文件中封装了调试相关函数，包括`dump_regs`，`dump_mem`，`dump_stack`和`dump_math`。

### 1.4 创建程序

1. 使用NASM编译汇编程序
```
nasm -f elf hello.asm
gcc -o hello hello.o 
```

2. 解决编译错误
    * "error: instruction not supported in 64-bit mode"的解决方法：nasm的格式选项改用elf代替elf64，gcc选项增加`-m32`
    * "relocation R_X86_64_32S against '.text' can not be used when making a PIE object; recompile with -fPIC"的解决方法：gcc选项增加`-no-pie`
    * "undefined reference to '\_printf'"的解决方法：使用nasm编译asm_io.asm时，增加`-d ELF_TYPE`选项。

3. 段
    * 已初始化的数据存储在`.data`段
    * 未初始化的数据存储在`.bss`段
    * 代码存储在`.text`段

4. `global`：汇编语言的label默认拥有internal scope，这意味着只有同一个模块的代码可以使用此label。globel指令使label拥有external scope，使得程序中任意模块都可使用此label。

5. `enter`指令创建一个栈帧，`leave`指令销毁一个栈帧。`enter`指令的第一个参数用来指示需要为局部变量申请的内存大小。`enter`和`leave`指令的等价代码如下所示：
```
; enter
push ebp
mov esp, ebp
sub firstPram, esp
; leave
mov esp, ebp
pop ebp
```

6. windows的目标文件是coff（Common Object File Format）格式的，linux的目标文件是elf（Executable and Linkable Format）格式。

7. `-l listing-file`选项可以让nasm生成一个包含汇编信息的list文件。该文件第2列是数据或代码在段中的偏移，注意这个偏移值不一定是最终形成整个程序时的真实偏移值，因为不同模块都可能在数据段定义了自己的label，在链接阶段，所有数据段的label定义汇总在一个数据段里，这时链接器需要重新计算各个label 的偏移。

8. 大小端
    * IBM框架、大部分RISC处理器和摩托罗拉处理器都是大端序，而Intel处理器是小端序。
    * 需要关注字节序的场景：
        * 当字节数据在不同主机间传输时
        * 当字节数据作为多字节整数写到内存，然后逐个字节读取时（或者相反）
    * 字节序对数组元素的顺序不影响，数组的第一个元素永远在最小的地址。但字节序对数组的每个单独元素还是会有影响（比如元素是多字节的整数时）。

