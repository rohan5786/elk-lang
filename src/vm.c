#include "vm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "error.h"
#include "memory.h"
#include "parse.h"
#include "value.h"

VM vm;

void init_stack() {
  vm.stack_capacity = 8192;
  vm.stack = malloc(sizeof(Value) * vm.stack_capacity);
  vm.top = vm.stack;  // pointing above top value (index 0) when empty
}

void push(Value val) {
  const int old_count = vm.top - vm.stack;
  if (vm.stack_capacity < old_count + 1) {
    const int old_capacity = vm.stack_capacity;
    vm.stack_capacity = ADD_CAPACITY(old_capacity);
    vm.stack = ADD_ARR(Value, vm.stack, vm.stack_capacity);
    vm.top = vm.stack + old_count;
  }

  *vm.top = val;  // set pointer (Value)
  vm.top++;       // increment index
}

Value pop() {
  vm.top--;        // decrement index
  return *vm.top;  // return pointer (Value)
}

void init_vm() { init_stack(); }

void free_vm() { free(vm.stack); }

/**
 * Switches through all OPCodes given
 * optimize memory management later
 */
static Result run() {
#define NEXT_BYTE() (*vm.instruction_ptr++)
// while(0) stuff is because of if/else ; termination on a {} function
// (elif/else never reached) ptr logic makes this faster because less movement
// of vm.top
#define BINARY_OP(op)                                              \
  do {                                                             \
    if (!IS_NUM(*(vm.top - 2)) || !IS_NUM(*(vm.top - 1))) {        \
      runtime_err(*(vm.top - 2), "both operands must be numbers"); \
      return RUNTIME_ERR;                                          \
    }                                                              \
    double b = pop().as.num;                                       \
    double a = pop().as.num;                                       \
    push(NUM_VAL(a op b));                                         \
  } while (0)
#define ADD_OP()                                                              \
  do {                                                                        \
    if (IS_NUM(*(vm.top - 2)) && IS_NUM(*(vm.top - 1))) {                     \
      double b = pop().as.num;                                                \
      double a = pop().as.num;                                                \
      push(NUM_VAL(a + b));                                                   \
    } else if (IS_STR(*(vm.top - 2)) && IS_STR(*(vm.top - 1))) {              \
      char* b = pop().as.str;                                                 \
      char* a = pop().as.str;                                                 \
      char* ab = malloc(strlen(a) + strlen(b) - 1);                           \
      if (ab == NULL) {                                                       \
        runtime_err(*(vm.top - 2),                                            \
                    "not enough memory to concatenate two strings");          \
        return RUNTIME_ERR;                                                   \
      }                                                                       \
      /* stupid removing extra quotes logic */                                \
      memcpy(ab, a, strlen(a) - 1);                                           \
      memcpy(ab + strlen(a) - 1, b + 1, strlen(b) - 1);                       \
      ab[strlen(a) + strlen(b) - 2] = '\0';                                   \
      push(STR_VAL(ab));                                                      \
      free(a);                                                                \
      free(b);                                                                \
    } else {                                                                  \
      runtime_err(*(vm.top - 2), "both operands must be numbers or strings"); \
      return RUNTIME_ERR;                                                     \
    }                                                                         \
  } while (0)

#define SINGLE_COMPARE(op)                                         \
  do {                                                             \
    if (!IS_NUM(*(vm.top - 2)) || !IS_NUM(*(vm.top - 1))) {        \
      runtime_err(*(vm.top - 2), "both operands must be numbers"); \
      return RUNTIME_ERR;                                          \
    }                                                              \
    double b = pop().as.num;                                       \
    double a = pop().as.num;                                       \
    push(NUM_VAL((a op b) ? 1.0 : 0.0));                           \
  } while (0)

#define MULT_COMPARE(op)                                                      \
  do {                                                                        \
    Value b = pop();                                                          \
    Value a = pop();                                                          \
    push(NUM_VAL((values_equal(a, NUM_VAL(1)) op values_equal(b, NUM_VAL(1))) \
                     ? 1.0                                                    \
                     : 0.0));                                                 \
  } while (0)

  while (1) {
#ifdef DEBUG_TRACE
    for (Value* slot = vm.stack; slot < vm.top; slot++) {
      printf("[");
      print_value(*slot);
      printf("], ");
    }
    printf("\n");
    disassemble_instruction(vm.code,
                            (int)(vm.instruction_ptr - vm.code->bytes));
#endif
    uint8_t cur_instruction;
    switch (cur_instruction = NEXT_BYTE()) {
      // UNKNOWN: change indeces to adapt or just be big enough
      case OP_GET_LOCAL: {
        uint16_t vm_stack_index =
            NEXT_BYTE() | (NEXT_BYTE() << 8);  // change eventually ?
        push(vm.stack[vm_stack_index]);        // re-getting it
        break;
      }
      case OP_SET_LOCAL: {
        uint16_t vm_stack_index =
            NEXT_BYTE() | (NEXT_BYTE() << 8);  // change eventually ?
        vm.stack[vm_stack_index] = *(vm.top - 1);
        break;
      }
      case OP_SET_INDEX: {
        const uint16_t stack_index = NEXT_BYTE() | (NEXT_BYTE() << 8);
        Value to_modify = vm.stack[stack_index];

        // working backwards; this is added last
        Value new_set_val = pop();
        int32_t index_val = GET_NUM(pop());

        if (to_modify.type == VAL_VEC) {
          Vector* vec = GET_VEC(to_modify);

          // TODO: fixed size arrays ---> dynamic arrays
          if (index_val < 0 || index_val >= vec->count) {
            runtime_err(NUM_VAL(index_val),
                        "expected unsigned 16-bit integer index value for type "
                        "'(fixed) vector'");
            return RUNTIME_ERR;
          }

          vec->items[index_val] = new_set_val;
        } else if (to_modify.type == VAL_STR) {
          char* str = GET_STR(to_modify);

          // bc of "" wrapping, strlen is 2 + what it should
          if (index_val < 0 || index_val >= (int)strlen(str) - 2) {
            runtime_err(NUM_VAL(index_val),
                        "expected unisgned 16-bit integer index value for "
                        "type 'str'");
            return RUNTIME_ERR;
          }
          // includes quotes; a ==> "a"
          if (!IS_STR(new_set_val) || strlen(GET_STR(new_set_val)) != 3) {
            runtime_err(new_set_val,
                        "expected single character length assignment for "
                        "string literal");
            return RUNTIME_ERR;
          }
          const char* new_set_val_str = GET_STR(new_set_val);
          const char letter = new_set_val_str[1];
          str[index_val + 1] = letter;  // to shift past first buffer "
        }
        break;
      }
      case OP_VECTOR: {
        // build YOUR VECTOR!
        uint16_t vec_size = NEXT_BYTE() | (NEXT_BYTE() << 8);
        Vector* vec = malloc(sizeof(Vector));
        init_vec(vec);
        vec->capacity = vec_size;
        vec->count = vec_size;
        vec->items = malloc(sizeof(Value) * vec_size);
        for (int i = vec_size - 1; i >= 0; i--) vec->items[i] = pop();

        push(VEC_VAL((struct Vector*)(vec)));
        break;
      }
      case OP_I8: {
        int8_t value = NEXT_BYTE();

        push(NUM_VAL((double)(value)));
        break;
      }
      case OP_I16: {
        uint16_t b0 = NEXT_BYTE();
        uint16_t b1 = NEXT_BYTE();

        int16_t value = b0 | (b1 << 8);

        push(NUM_VAL((double)(value)));
        break;
      }
      case OP_I32: {
        uint32_t b0 = NEXT_BYTE();
        uint32_t b1 = NEXT_BYTE();
        uint32_t b2 = NEXT_BYTE();
        uint32_t b3 = NEXT_BYTE();

        int32_t value = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);

        push(NUM_VAL((double)(value)));
        break;
      }
      case OP_I64: {
        uint64_t b0 = NEXT_BYTE();
        uint64_t b1 = NEXT_BYTE();
        uint64_t b2 = NEXT_BYTE();
        uint64_t b3 = NEXT_BYTE();
        uint64_t b4 = NEXT_BYTE();
        uint64_t b5 = NEXT_BYTE();
        uint64_t b6 = NEXT_BYTE();
        uint64_t b7 = NEXT_BYTE();

        int64_t value = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24) | (b4 << 32) |
                        (b5 << 40) | (b6 << 48) | (b7 << 56);

        push(NUM_VAL((double)(value)));
        break;
      }
      case OP_F32: {
        uint32_t b0 = NEXT_BYTE();
        uint32_t b1 = NEXT_BYTE();
        uint32_t b2 = NEXT_BYTE();
        uint32_t b3 = NEXT_BYTE();

        const uint32_t bits = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
        float value = *(float*)(&bits);

        push(NUM_VAL((double)(value)));
        break;
      }
      case OP_F64: {
        uint64_t b0 = NEXT_BYTE();
        uint64_t b1 = NEXT_BYTE();
        uint64_t b2 = NEXT_BYTE();
        uint64_t b3 = NEXT_BYTE();
        uint64_t b4 = NEXT_BYTE();
        uint64_t b5 = NEXT_BYTE();
        uint64_t b6 = NEXT_BYTE();
        uint64_t b7 = NEXT_BYTE();

        const uint64_t bits = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24) |
                              (b4 << 32) | (b5 << 40) | (b6 << 48) | (b7 << 56);
        double value = *(double*)(&bits);

        push(NUM_VAL((double)(value)));
        break;
      }
      case OP_STR: {
        uint32_t str_len = NEXT_BYTE() | (NEXT_BYTE() << 8) |
                           (NEXT_BYTE() << 16) | (NEXT_BYTE() << 24);
        char* str = malloc(str_len + 1);
        for (int i = 0; i < str_len; i++) {
          const unsigned char ch = NEXT_BYTE();
          str[i] = ch;  // unsigned char
        }
        str[str_len] = '\0';

        push(STR_VAL(str));
        break;
      }
      case OP_ADD: {
        ADD_OP();
        break;
      }
      case OP_SUB: {
        BINARY_OP(-);
        break;
      }
      case OP_MULT: {
        BINARY_OP(*);
        break;
      }
      case OP_DIV: {
        BINARY_OP(/);
        break;
      }
      case OP_NEGATE: {
        Value old = pop();
        if (!IS_NUM(*(vm.top - 1))) {
          runtime_err(*(vm.top - 1), "single operand must be a number");
          return RUNTIME_ERR;
        }
        push(NUM_VAL(-old.as.num));
        break;
      }
      case OP_LESS: {
        SINGLE_COMPARE(<);
        break;
      }
      case OP_LESS_EQL: {
        SINGLE_COMPARE(<=);
        break;
      }
      case OP_EQUAL: {
        SINGLE_COMPARE(==);
        break;
      }
      case OP_NOT_EQL: {
        SINGLE_COMPARE(!=);
        break;
      }
      case OP_GREATER_EQL: {
        SINGLE_COMPARE(>=);
        break;
      }
      case OP_GREATER: {
        SINGLE_COMPARE(>);
        break;
      }
      case OP_AND: {
        MULT_COMPARE(&&);
        break;
      }
      case OP_OR: {
        MULT_COMPARE(||);
        break;
      }
      case OP_XOR: {
        MULT_COMPARE(^);
        break;
      }
      case OP_JMP: {
        uint16_t full_offset = NEXT_BYTE() | (NEXT_BYTE() << 8);
        vm.instruction_ptr += full_offset;
        break;
      }
      case OP_TRUE_JMP: {
        // find the amnt of bytes of instruction to skip
        uint16_t full_offset =
            NEXT_BYTE() | (NEXT_BYTE() << 8);  // LSB is added first
        Value val = pop();
        // total offset to where we start on the next valid instruction set
        const bool true_eq =
            (!(IS_NUM(val) || IS_NULL(val)) || GET_NUM(val) != 0.0);
        if (true_eq) vm.instruction_ptr += full_offset;
        break;
      }
      case OP_FALSE_JMP: {
        uint16_t full_offset = NEXT_BYTE() | (NEXT_BYTE() << 8);
        Value val = pop();
        const bool true_eq =
            (!(IS_NUM(val) || IS_NULL(val)) || GET_NUM(val) != 0.0);
        if (!true_eq) vm.instruction_ptr += full_offset;
        break;
      }
      case OP_LOOP: {
        uint16_t full_offset = NEXT_BYTE() | (NEXT_BYTE() << 8);
        vm.instruction_ptr -= full_offset;
        break;
      }
      case OP_RETURN: {
        print_value(pop());
        printf("\n");
        return OK;
      }
    }
  }

#undef NEXT_BYTE
#undef BINARY_OP
#undef SINGLE_COMPARE
#undef MULT_COMPARE
}

Result interpret(const char* source) {
  Code code;
  init_code(&code);

  if (!compile(source, &code)) {
    free_code(&code);
    return COMPILE_ERR;
  }

  vm.code = &code;
  vm.instruction_ptr = vm.code->bytes;

  Result rs = run();
  free_code(&code);
  return rs;
}