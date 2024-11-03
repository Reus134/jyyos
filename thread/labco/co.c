#include "co.h"
#include <stdlib.h>

#define STACK_SIZE 64
typedef unsigned char           uint8_t;

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct jump_buf
{
    //前6个参数
    //rbx, rsp, rbp, r12, r13, r14, r15
    unsigned int rbx;
    unsigned int rsp;
    unsigned int rbp;
    unsigned int r12;
    unsigned int r13;
    unsigned int r14;
    unsigned int r15;
};

struct co {
    char *name;
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;

    enum co_status status;  // 协程的状态
    struct co *    waiter;  // 是否有其他协程在等待当前协程
    struct jump_buf       context; // 寄存器现场 (setjmp.h)
    uint8_t        stack[STACK_SIZE]; // 协程的栈
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *cur_co = malloc(sizeof(struct co)); 
    cur_co->status = CO_NEW;
    cur_co->name = name;
    cur_co->arg = arg;
    cur_co->func=func;
    cur_co->waiter = NULL;
    return cur_co;
}1

/*co_yield 会将当前运行协程的寄存器保存到共享内存中，
然后选择一个另一个协程，将寄存器加载到 CPU 上，
就完成了 “状态机的切换”；*/


void co_wait(struct co *co) {
    while () {
        
    }
}

/*
1.为每一个协程分配独立的堆栈；堆栈顶的指针由 %rsp 寄存器确定；
2.在 co_yield 发生时，将寄存器保存到属于该协程的 struct co 中 (包括 %rsp)；
3.切换到另一个协程执行，找到系统中的另一个协程，然后恢复它 struct co 中的寄存器现场 (包括 %rsp)。
*/

/*
    asm(
        内嵌汇编指令
        ：输出操作数
        ：输入操作数
        ：破坏描述
    );
*/
void co_yield() {
    // 把下一个栈指针 给目前的rsp 下一个函数的地址给*next_rip
    asm volatile (
        "mov (next_rsp), $rsp"
        "jmp *(next_rip)"
    )
}
