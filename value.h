#ifndef ELK_LANG_VALUE_H
#define ELK_LANG_VALUE_H

typedef double Value;

typedef struct
{
    int capacity;
    int count;
    Value *values;
} ValueArray;

void init_value_arr(ValueArray *arr);
void write_value_arr(ValueArray *arr, Value val);
void free_value_arr(ValueArray *arr);
void print_value(Value val);

#endif