#include <stdio.h>
#include "co.h"

int count = 1; // 协程之间共享

// void entry(void *arg) {
//   for (int i = 0; i < 5; i++) {
//     printf("%s[%d] ", (const char *)arg, count++);
//     co_yield();
//   }
// }

// static void work_loop(void *arg) {
//     const char *s = (const char*)arg;
//     for (int i = 0; i < 100; ++i) {
//         printf("%s%d  ", s, get_count());
//         add_count();
//         co_yield();
//     }
// }

// static void work(void *arg) {
//     work_loop(arg);
// }

void entry(void *arg) {
  for (int i = 0 ;i < 1 ; i++) {
    printf("i = %d arg = %s \n", i, (const char *)arg);
    co_yield();
  }
}

int main() {
  struct co *co1 = co_start("co1", entry, "a");
  struct co *co2 = co_start("co2", entry, "b");
  co_wait(co1);
  co_wait(co2);
  printf("Done\n");
}