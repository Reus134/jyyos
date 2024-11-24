#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdint.h>

#define STACK_SIZE 64 * 1024
#define CO_MAX_NUM 32
typedef unsigned char uint8_t;

struct co *current = NULL;
struct co *active_cos[10] = {0};
int active_co_numbers = 0;
int current_index = 0;

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

struct co *co_start(const char *name, void (*func)(void *), void *arg) {

    if (active_co_numbers == 0) {
        struct co *main_co = malloc(sizeof(struct co)); 
        main_co->status = CO_NEW;
        main_co->name = "main";
        main_co->waiter = NULL;
        active_cos[current_index] = main_co;
    }

    struct co *init_co = malloc(sizeof(struct co)); 
    init_co->status = CO_NEW;
    init_co->name = name;
    init_co->arg = arg;
    init_co->func = func;
    init_co->waiter = NULL;
    //共享内存里的current 指针指向cur_co
    //current = cur_co;
    active_co_numbers++;
    active_cos[active_co_numbers] = init_co;
    // if(setjmp(init_co->context) == 0) {
    //     printf("%s init setjmp\n",init_co->name);
    // } else {
    //     active_cos[current_index]->func(active_cos[current_index]->arg);
    //     active_cos[current_index]->status = CO_DEAD;
    //     active_co_numbers--;
    //     if(!active_co_numbers) {
    //         //回主协程（main）
    //         longjmp(active_cos[0]->context, 1);
    //     }
    //     co_yield();
    // }

    return init_co;
}

/*
1.此时协程已经结束 (func 返回).这是完全可能的。此时,co_wait 应该直接回收资源。
2.此时协程尚未结束，因此 co_wait 不能继续执行，必须调用 co_yield 切换到其他协程执行，直到协程结束后唤醒
*/
// current需要等待co执行完 当前协程需要等待co执行
void co_wait(struct co *co) {
    while(co->status != CO_DEAD) {
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

void co_yield() {
    //保留当前的context
    int ret = -1;
    //ret = setjmp(active_cos[current_index]->context);
    int old_index = current_index;
    //选择的协程是新创建的，此时该协程还没有执行过任何代码，我们需要首先执行 stack_switch_call 切换栈，然后开始执行协程的代码；
    //选择的协程是调用 yield() 切换出来的，此时该协程已经调用过 setjmp 保存寄存器现场，我们直接 longjmp 恢复寄存器现场即可。
    current_index = get_random_except_current(current_index, active_co_numbers);
    struct co *current_co = active_cos[current_index];
    //struct co* current_co = active_cos[current_index];
    ret = setjmp(active_cos[old_index]->context);
    
    active_cos[old_index]->status = CO_WAITING;
    if (ret == 0) {
        if (current_co->status == CO_NEW) {
            current_co->status = CO_RUNNING;
            stack_switch_call((uintptr_t)(current_co->stack + STACK_SIZE),(uintptr_t)(current_co->func), (uintptr_t)(current_co->arg));
        } else {
            active_cos[current_index]->status = CO_RUNNING;
            longjmp(active_cos[current_index]->context, 1);
        }
    } else {
        printf("co index = %d return to this\n",current_index);
        active_cos[current_index]->status = CO_DEAD;
    }
}
