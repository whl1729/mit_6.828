# 《MIT 6.828 Lab 1 Exercise 3》实验报告

## 题目

> Exercise 3. Take a look at the [lab tools guide](https://pdos.csail.mit.edu/6.828/2017/labguide.html), especially the section on GDB commands. Even if you're familiar with GDB, this includes some esoteric GDB commands that are useful for OS work.

> Set a breakpoint at address 0x7c00, which is where the boot sector will be loaded. Continue execution until that breakpoint. Trace through the code in boot/boot.S, using the source code and the disassembly file obj/boot/boot.asm to keep track of where you are. Also use the x/i command in GDB to disassemble sequences of instructions in the boot loader, and compare the original boot loader source code with both the disassembly in obj/boot/boot.asm and GDB.

> Trace into bootmain() in boot/main.c, and then into readsect(). Identify the exact assembly instructions that correspond to each of the statements in readsect(). Trace through the rest of readsect() and back out into bootmain(), and identify the begin and end of the for loop that reads the remaining sectors of the kernel from the disk. Find out what code will run when the loop is finished, set a breakpoint there, and continue to that breakpoint. Then step through the remainder of the boot loader.

> Be able to answer the following questions:
* At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?
* What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?
* Where is the first instruction of the kernel?
* How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?

## 解答

Exercise 3包含两部分：其一是使用GDB跟踪代码，其二是回答4个问题。

### 一、使用GDB跟踪代码

1. 我已阅读完练习中提供的GDB材料，并输出[学习笔记](read_lab_tools_guide.md)。

2. 跟踪`boot.S`的代码
    * 在地址0x7c00处设置断点，这是boot loader第一条指令的位置。
    * 使用si命令跟踪代码，可知`boot.S`文件中主要是做了以下事情：初始化段寄存器、使能A20、从实模式跳到虚模式（需要设置LGT和cr0寄存器），最后调用bootmain函数。
    * 题目中还要求我们比较`boot.S`，`boot.asm`与GDB中的代码差异，我观察到的差异有： `boot.S`的指令含有表示长度的b,w,l等后缀，而`boot.asm`和GDB没有；同样一条指令，`boot.S`和GDB是操作ax寄存器，而`boot.asm`却是操作%eax。
```
xorw %ax, %ax   // boot.S
xor %eax, %eax  // boot.asm
xor %ax, %ax    // GDB
```

3. 接下来是跟踪`main.c`的代码
    * 在bootmain函数的起始地址（0x7d15）处设置断点。bootmain函数开头定义了两个局部变量ph和eph，从汇编代码发现gcc分别用%ebx和%esi这两个寄存器来保存它们的值，而不是从栈中开辟空间来保存。从下面0x7d4c处的代码还可以发现ph指针加1对应地址偏移32个字节（Proghdr结构体占32个字节）。
```
// The C codes:
// ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
// eph = ph + ELFHDR->e_phnum;
7d3a:	a1 1c 00 01 00       	mov    0x1001c,%eax
7d3f:	0f b7 35 2c 00 01 00 	movzwl 0x1002c,%esi
7d46:	8d 98 00 00 01 00    	lea    0x10000(%eax),%ebx
7d4c:	c1 e6 05             	shl    $0x5,%esi
7d4f:	01 de                	add    %ebx,%esi
```
    * 跟踪readseg函数：接着调用readseg函数读取第一页磁盘内存，在readseg函数里需要调用readsect函数。 
    * 跟踪for循环

### 二、回答问题

### 备注

1. Test对两个参数(目标，源)执行AND逻辑操作，并根据结果设置标志寄存器，结果本身不会保存。

2. x86 EFLAGS寄存器各状态标志的含义：
    * CF(bit 0) [Carry flag]: 若算术操作产生的结果在最高有效位(most-significant bit)发生进位或借位则将其置1，反之清零。这个标志指示无符号整型运算的溢出状态，这个标志同样在多倍精度运算(multiple-precision arithmetic)中使用。
    * PF(bit 2) [Parity flag]: 如果结果的最低有效字节(least-significant byte)包含偶数个1位则该位置1，否则清零。
    * AF(bit 4) [Adjust flag]: 如果算术操作在结果的第3位发生进位或借位则将该标志置1，否则清零。这个标志在BCD(binary-code decimal)算术运算中被使用。
    * ZF(bit 6) [Zero flag]: 若结果为0则将其置1，反之清零。
    * SF(bit 7) [Sign flag]: 该标志被设置为有符号整型的最高有效位。(0指示结果为正，反之则为负)
    * OF(bit 11) [Overflow flag]: 如果整型结果是较大的正数或较小的负数，并且无法匹配目的操作数时将该位置1，反之清零。这个标志为带符号整型运算指示溢出状态。

3. readsect函数中调用了insl函数，而insl函数的实现是一个内联汇编语句。这个[stackflow网站](https://stackoverflow.com/questions/38410829/why-cant-find-the-insl-instruction-in-x86-document)解释了insl函数的作用：“That function will read cnt dwords from the input port specified by port into the supplied output array addr.”。关于内联汇编的介绍见[Brennan's Guide to Inline Assembly](http://www.delorie.com/djgpp/doc/brennan/brennan_att_inline_djgpp.html)和[GCC内联汇编基础](https://www.jianshu.com/p/1782e14a0766)。

#### 疑问

1. 如何查看某个地址对应的符号（函数名或变量名）？网上说使用`info symbol addr`命令，但我使用时提示"No symbol matches 0x7d15."

