#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>

#define STACK_SIZE 64 * 1024
#define CO_MAX_NUM 32
typedef unsigned char uint8_t;

enum co_status {
    CO_NEW = 1, // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  const char *name;
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co*     waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]; // 协程的栈
};

//维护一个 co结构体实例的双向链表
typedef struct co_node
{
    struct co_node *pre;
    struct co *co;
    struct co_node *next;
}co_node;

struct co *current = NULL;
co_node* co_nodes = NULL;
int co_numbers = 0;

static co_node* insert_co(struct co *coroutine) {
    co_node *new_co_node = malloc(sizeof(co_node));
    new_co_node->co = coroutine;
    if (co_nodes == NULL) {
        //初始化 均指向自己
        new_co_node->pre = new_co_node;
        new_co_node->next = new_co_node;
        //co_nodes指针指向当前的节点
        co_nodes = new_co_node;
        return new_co_node;
    }
    
    co_nodes->next->pre = new_co_node;
    new_co_node->next = co_nodes->next;
    new_co_node->pre = co_nodes;
    co_nodes->next = new_co_node;
    co_nodes = new_co_node;
    co_numbers++;
    return new_co_node;
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    if (current == NULL) {
        struct co *init_co = malloc(sizeof(struct co));
        init_co->name = "main";
        init_co->status = CO_RUNNING;
        current = init_co;
    }
    struct co *coroutine = malloc(sizeof(struct co)); 
    coroutine->status = CO_NEW;
    coroutine->name = name;
    coroutine->arg = arg;
    coroutine->func = func;
    coroutine->waiter = NULL;

    // 插入当前的创建的co到co_list
    insert_co(coroutine);
    return coroutine;
}

/*
1.此时协程已经结束 (func 返回).这是完全可能的。此时,co_wait 应该直接回收资源。
2.此时协程尚未结束，因此 co_wait 不能继续执行，必须调用 co_yield 切换到其他协程执行，直到协程结束后唤醒
*/
// current需要等待co执行完 当前协程需要等待co执行
void co_wait(struct co *co) {
    while(co->status != CO_DEAD) {
        co->status = CO_WAITING;
        co->waiter = current;
        co_yield();
        //co->status = CO_DEAD;
    }
    // 走到这说明一家CO_DEAD
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

static int get_random_except_current(int index, int bound) {
    int num;
    do {
        num = rand() % bound + 1; // 生成 [1, bound] 范围的随机数
    } while (num == index);        // 排除 index
    return num;
}

static inline void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
  asm volatile (
#if __x86_64__
    "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
      : : "b"((uintptr_t)sp), "d"(entry), "a"(arg) : "memory"
#else
    "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
      : : "b"((uintptr_t)sp - 8), "d"(entry), "a"(arg) : "memory"
#endif
  );
}

/*
 * 从调用的指定函数返回，并恢复相关的寄存器。此时协程执行结束，以后再也不会执行该协程的上下文。这里需要注意的是，其和上面并不是对称的，因为调用协程给了新创建的选中协程的堆栈，则选中协程以后就在自己的堆栈上执行，永远不会返回到调用协程的堆栈。
 */
static inline void restore_return() {
  asm volatile(
#if __x86_64__
      "movq 0(%%rsp), %%rcx"
      :
      :
#else
      "movl 4(%%esp), %%ecx"
      :
      :
#endif
  );
}


void co_yield() {
    //保留当前的context
    int ret = -1;
    int i = 0;
    //struct co *current_co;
    //1.选择的协程是新创建的，此时该协程还没有执行过任何代码，我们需要首先执行 stack_switch_call 切换栈，然后开始执行协程的代码；
    //2.选择的协程是调用 yield() 切换出来的，此时该协程已经调用过 setjmp 保存寄存器现场，我们直接 longjmp 恢复寄存器现场即可。
    //遍历链表，选择下一个co_node,这个co_node需要是CO_RUNNING 或者是CO_NEW
    
    //第一步 保留此时进来co_yeild的
    ret = setjmp(current->context);
    //第二步 选择一个CO_NEW 或者 CO_RUNING状态的
    for(i; i < co_numbers; i++) {
        co_nodes = co_nodes->next;
        if (co_nodes->next->co->status == CO_NEW || co_nodes->next->co->status == CO_RUNNING) {
            current = co_nodes->co;
            break;
        }
    }

    assert(current);

    if (i == co_numbers) {
        printf("no co is new or running\n");
    }
    if (ret == 0) {
        if (current->status == CO_NEW) {
            current->status = CO_RUNNING;
            stack_switch_call(current->stack + STACK_SIZE, current->func, current->arg);
            restore_return();
            //上面这个函数需要返回处理的
            current->status = CO_DEAD;

            if (current->waiter) {
                current->waiter->status = CO_RUNNING;
            }
            co_yield ();
        } else {
            current->status = CO_RUNNING;
            longjmp(current->context, 1);
        }
    } else {
        printf("co index = %s return to this\n", current->name);
    }
}
