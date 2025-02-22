#include <common.h>

static size_t align_to_power_of_two(size_t n) {
    if (n == 0) {
        return 1; // 0 对齐到 1（2^0）
    }
    
    // 检查 n 是否已经是 2 的幂
    if ((n & (n - 1)) == 0) {
        return n; // 直接返回自身
    }
    
    // 找到最高有效位的位置
    size_t position = 0;
    size_t temp = n;
    while (temp != 0) {
        temp >>= 1;
        position++;
    }
    
    // 检查是否溢出（例如：n = SIZE_MAX）
    if (position >= sizeof(size_t) * 8) {
        return 0; // 无法对齐，返回 0 表示错误
    }
    
    return (size_t)1 << position; // 2^(position)
}

static void *kalloc(size_t size) {
  return NULL;
}

static void kfree(void *ptr) {
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
}

MODULE_DEF(pmm) = {
  .init  = pmm_init,
  .alloc = kalloc,
  .free  = kfree,
};
