/*
在这个实验中，我们实现轻量级的用户态携谐协程 (coroutine，“协同程序”)，
也称为 green threads、user-level threads，
可以在一个不支持线程的操作系统上实现共享内存多任务并发。
即我们希望实现 C 语言的 “函数”，它能够：
被 start() 调用，从头开始运行；
在运行到中途时，调用 yield() 被 “切换” 出去；
稍后有其他协程调用 yield() 后，选择一个先前被切换的协程继续执行。
*/

/*
struct co *co_start(const char *name, void (*func)(void *), void *arg);
void       co_yield();
void       co_wait(struct co *co);
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
//#include "co-test.h"
#define LOCAL_MACHINE
#ifdef LOCAL_MACHINE
  #define debug(...) printf(__VA_ARGS__)
#else
  #define debug()
#endif

// 协程共享内存 需要有独立的堆栈和寄存器


// func是新创建的协程丛func函数开始执行，arg是func的参数

struct co *co_start(const char *name, void (*func)(void *), void *arg);

void co_yield();

// 在被等待的协程结束后，co_wait()返回前
void co_wait(struct co *co);