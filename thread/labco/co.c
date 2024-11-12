#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>

#define STACK_SIZE 64
#define CO_MAX_NUM 32
typedef unsigned char uint8_t;
//当前
struct co *current;

enum co_status {
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
    char *name;
    void (*func)(void *); // co_start 指定的入口地址和参数
    void *arg;

    enum co_status status;  // 协程的状态
    struct co*     waiter;  // 是否有其他协程在等待当前协程
    jmp_buf        context; // 寄存器现场 (setjmp.h)
    uint8_t        stack[STACK_SIZE]; // 协程的栈
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *cur_co = malloc(sizeof(struct co)); 
    cur_co->status = CO_NEW;
    cur_co->name = name;
    cur_co->arg = arg;
    cur_co->func=func;
    cur_co->waiter = NULL;
    //共享内存里的current 指针指向cur_co
    //current = cur_co;
    return cur_co;
}

/*
1.此时协程已经结束 (func 返回).这是完全可能的。此时,co_wait 应该直接回收资源。
2.此时协程尚未结束，因此 co_wait 不能继续执行，必须调用 co_yield 切换到其他协程执行，直到协程结束后唤醒
*/
// current需要等待co执行完 当前协程需要等待co执行
void co_wait(struct co *co) {
    current->status = CO_WAITING;
    current->waiter = co;
    while(co->status != CO_DEAD) {
        co_yield();
        co->status = CO_DEAD;
    }
    free(co);
}

/*
1.为每一个协程分配独立的堆栈；堆栈顶的指针由 %rsp 寄存器确定；
2.在 co_yield 发生时，将寄存器保存到属于该协程的 struct co 中 (包括 %rsp)；
3.切换到另一个协程执行，找到系统中的另一个协程，然后恢复它 struct co 中的寄存器现场 (包括 %rsp)。
*/
/*co_yield 会将当前运行协程的寄存器保存到共享内存中，
然后选择一个另一个协程，将寄存器加载到 CPU 上，
就完成了 “状态机的切换”；*/
/*
    asm(
        内嵌汇编指令
        ：输出操作数
        ：输入操作数
        ：破坏描述
    );
*/

void co_yield() {
    //保留当前的context
    int val = setjmp(current->context);
    if (val == 0) {
        // 随机选择一个协程 搞个队列？
        if (current->waiter != NULL) {
            current = current->waiter;
        }
        
        current->status = CO_RUNNING;
        //执行这个函数
        current->func(NULL);
        //函数返回则long jmp回来
        current->status = CO_DEAD;
        longjmp(current->context, 1);
    } else {
        // 如果某个地方通过long jump回到了这里
        current->status = CO_DEAD;
        printf("return to this\n");
    }
}
