.globl _start
	
.text
_start:// 程序的入口
	movl $1, %eax // 系统调用号为1，表示退出
	movl $0, %ebx # return value //递给exit系统调用的返回值存储在%ebx寄存器中，
 //因此将其设置为0表示正常退出。
	int $0x80
// 使用int $0x80 执行syscall；
