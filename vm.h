#ifndef ELK_LANG_VM_H
#define ELK_LANG_VM_H

// #define STACK_INIT 8192
// #define STACK_MAX 1048576

#include "code.h"
#include "value.h"

typedef struct
{
    Code *code;
    uint8_t *instruction_ptr;
    Value *stack;
    Value *top;
    int stack_capacity;
} VM;

typedef enum
{
    OK,
    COMPILE_ERR,
    RUNTIME_RR,
} Result;

void init_stack();
void push(Value val);
Value pop();
void init_vm();
void free_vm();
Result interpret(Code *code);

#endif