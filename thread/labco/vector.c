zxcds

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void *data;           // 存储数据的指针
    size_t elem_size;     // 单个元素大小
    size_t capacity;      // 当前容量
    size_t length;        // 当前长度
} DynamicArray;

DynamicArray *da_init(size_t elem_size, size_t initial_capacity) {
    DynamicArray *array = (DynamicArray *)malloc(sizeof(DynamicArray));
    if (!array) {
        fprintf(stderr, "Memory allocation failed for DynamicArray\n");
        exit(EXIT_FAILURE);
    }

    array->data = malloc(elem_size * initial_capacity);
    if (!array->data) {
        fprintf(stderr, "Memory allocation failed for data\n");
        free(array);
        exit(EXIT_FAILURE);
    }

    array->elem_size = elem_size;
    array->capacity = initial_capacity;
    array->length = 0;

    return array;
}

void da_push_back(DynamicArray *array, void *element) {
    if (array->length == array->capacity) {
        // 扩容：容量翻倍
        array->capacity *= 2;
        array->data = realloc(array->data, array->elem_size * array->capacity);
        if (!array->data) {
            fprintf(stderr, "Memory reallocation failed\n");
            exit(EXIT_FAILURE);
        }
    }

    // 复制元素到数组末尾
    void *target = (char *)array->data + array->length * array->elem_size;
    memcpy(target, element, array->elem_size);
    array->length++;
}

void *da_get(DynamicArray *array, size_t index) {
    if (index >= array->length) {
        fprintf(stderr, "Index out of bounds\n");
        return NULL;
    }
    return (char *)array->data + index * array->elem_size;
}

void da_free(DynamicArray *array) {
    free(array->data);
    free(array);
}
