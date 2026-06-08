#include "vm.h"
#include "debug.h"
#include "memory.h"
#include <stdio.h>

VM vm;

void init_stack()
{
    vm.stack_capacity = 8192;
    vm.stack = malloc(sizeof(Value) * vm.stack_capacity);
    vm.top = vm.stack; // pointing above top value (index 0) when empty
}

void push(Value val)
{
    const int old_count = vm.top - vm.stack;
    if (vm.stack_capacity < old_count + 1) 
    {
        const int old_capacity = vm.stack_capacity;
        vm.stack_capacity = ADD_CAPACITY(old_capacity);
        vm.stack = ADD_ARR(Value, vm.stack, vm.stack_capacity);
        vm.top = vm.stack + old_count;
    }

    *vm.top = val; // set pointer (Value)
    vm.top++; // increment index 
}

Value pop()
{
    vm.top--;       // decrement index
    return *vm.top; // return pointer (Value)
}

void init_vm()
{
    init_stack();
}

void free_vm()
{
    free(vm.stack);
}

/**
 * Switches through all OPCodes given
 * optimize memory management later
 */
static Result run()
{
#define NEXT_BYTE() (*vm.instruction_ptr++)
#define NEXT_CONST() (vm.code->constants.values[NEXT_BYTE()])
// while(0) stuff is because of if/else ; termination on a {} function (elif/else never reached)
// ptr logic makes this faster because less movement of vm.top
#define BINARY_OP(op) do { \
        vm.top--; \
        *(vm.top - 1) = *(vm.top - 1) op *(vm.top); \
    } while (0)

    while (1) 
    {
        #ifdef DEBUG_TRACE
            for (Value *slot = vm.stack; slot < vm.top; slot++)
            {
                printf("[");
                print_value(*slot);
                printf("], ");
            }
            printf("\n");
            disassemble_instruction(vm.code, (int)(vm.instruction_ptr - vm.code->bytes));
        #endif
        uint8_t cur_instruction;
        switch (cur_instruction = NEXT_BYTE())
        {
            case CONSTANT_LONG: {
                const int const_index = NEXT_BYTE() | (NEXT_BYTE() << 8) | (NEXT_BYTE() << 16);
                push(vm.code->constants.values[const_index]);
                break;
            }
            case CONSTANT: {
                push(NEXT_CONST());
                break;
            }
            case ADD: {
                BINARY_OP(+);
                break;
            }
            case SUB: {
                BINARY_OP(-);
                break;
            }
            case MULT: {
                BINARY_OP(*);
                break;
            }
            case DIV: {
                BINARY_OP(/);
                break;
            }
            case NEGATE: {
                *(vm.top - 1) = -(*(vm.top - 1));
                break;
            }
            case RETURN: {
                print_value(pop());
                printf("\n");
                return OK;
            }
        }
    }

#undef NEXT_BYTE
#undef NEXT_CONST
#undef BINARY_OP
}

Result interpret(Code *code)
{
    vm.code = code;
    vm.instruction_ptr = code->bytes;
    return run();
}