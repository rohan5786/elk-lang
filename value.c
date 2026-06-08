#include "value.h"
#include "memory.h"
#include <stdio.h>

void init_value_arr(ValueArray *arr)
{
    arr->capacity = 0;
    arr->count = 0;
    arr->values = NULL;
}

void write_value_arr(ValueArray *arr, Value val)
{
    if (arr->capacity < arr->count + 1)
    {
        int old_capacity = arr->capacity;
        arr->capacity = ADD_CAPACITY(old_capacity);
        arr->values = ADD_ARR(Value, arr->values, arr->capacity);
    }

    arr->values[arr->count] = val;
    arr->count++;
}

void free_value_arr(ValueArray *arr)
{
    ADD_ARR(Value, arr->values, 0);
    init_value_arr(arr);
}

void print_value(Value val)
{
    printf("Value: %.0lf", val);
}