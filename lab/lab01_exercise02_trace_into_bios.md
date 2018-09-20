# 《MIT 6.828 Lab 1 Exercise 2》实验报告

## 题目

> Exercise 2. Use GDB's si (Step Instruction) command to trace into the ROM BIOS for a few more instructions, and try to guess what it might be doing. You might want to look at [Phil Storrs I/O Ports Description](http://web.archive.org/web/20040404164813/members.iweb.net.au/~pstorr/pcbook/book2/book2.htm), as well as other materials on the [6.828 reference materials page](https://pdos.csail.mit.edu/6.828/2017/reference.html). No need to figure out all the details - just the general idea of what the BIOS is doing first.

## 解答

使用si命令得到的前22条汇编指令如下。虽然能看懂每条指令的字面意思，但看不懂具体实现的功能，后来参考[myk的6.828 Lab1](https://zhuanlan.zhihu.com/p/36926462)大致理解了基本功能：设置ss和esp寄存器的值，打开A20门（为了后向兼容老芯片）、进入保护模式（需要设置cr0寄存器的PE标志）。
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

## 备注

1. CS（CodeSegment）和IP（Instruction Pointer）寄存器一起用于确定下一条指令的地址。

2. CLI：Clear Interupt，禁止中断发生。STL：Set Interupt，允许中断发生。CLI和STI是用来屏蔽中断和恢复中断用的，如设置栈基址SS和偏移地址SP时，需要CLI，因为如果这两条指令被分开了，那么很有可能SS被修改了，但由于中断，而代码跳去其它地方执行了，SP还没来得及修改，就有可能出错。

3. CLD: Clear Director。STD：Set Director。在字行块传送时使用的，它们决定了块传送的方向。CLD使得传送方向从低地址到高地址，而STD则相反。

4. 汇编语言中，CPU对外设的操作通过专门的端口读写指令来完成，读端口用IN指令，写端口用OUT指令。进一步理解“端口”的概念可以参考博客[理解“统一编址与独立编址、I/O端口与I/O内存”](https://my.oschina.net/wuying/blog/53419)。

5. LIDT: 加载中断描述符。LGDT：加载全局描述符。

6. 第一条指令：`[f000:fff0] 0xffff0:    ljmp   $0xf000,$0xe05b`
    * PC开始运行时，CS = 0xf000，IP = 0xfff0，对应物理地址为0xffff0。（计算公式： physical address = 16 * segment + offset）
    * 第一条指令做了jmp操作，跳到物理地址为0xfe05b的位置。

7. 控制寄存器：控制寄存器（CR0～CR3）用于控制和确定处理器的操作模式以及当前执行任务的特性。CR0中含有控制处理器操作模式和状态的系统控制标志；CR1保留不用；CR2含有导致页错误的线性地址；CR3中含有页目录表物理内存基地址，因此该寄存器也被称为页目录基地址寄存器PDBR（Page-Directory Base address Register）。
    * CR0的4个位：扩展类型位ET、任务切换位TS、仿真位EM和数学存在位MP用于控制80x86浮点（数学）协处理器的操作。
    * CR0的位0是PE（Protection Enable）标志。当设置该位时即开启了保护模式；当复位时即进入实地址模式。这个标志仅开启段级保护，而并没有启用分页机制。若要启用分页机制，那么PE和PG标志都要置位。
    * CR0的位31是PG（Paging，分页）标志。当设置该位时即开启了分页机制；当复位时则禁止分页机制，此时所有线性地址等同于物理地址。在开启这个标志之前必须已经或者同时开启PE标志。即若要启用分页机制，那么PE和PG标志都要置位。
