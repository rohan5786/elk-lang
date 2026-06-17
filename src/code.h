#ifndef CODE_H
#define CODE_H

#include "basic.h"
#include "value.h"

/**
 * In bytecode format, each instruction must have a one-byte operation code
 * (e.g. add/subtract) Defining th eones we want for now
 */
typedef enum {
  OP_CONSTANT_LONG,
  OP_CONSTANT,
  // Binary ops.
  OP_ADD,
  OP_SUB,
  OP_MULT,
  OP_DIV,
  OP_NEGATE,
  // single eval conditionals
  OP_LESS,
  OP_LESS_EQL,
  OP_NOT_EQL,
  OP_EQUAL,
  OP_GREATER_EQL,
  OP_GREATER,
  // mult eval cond
  OP_AND,
  OP_OR,
  OP_XOR,
  // cond jumping
  OP_JMP,
  OP_TRUE_JMP,
  OP_FALSE_JMP,
  // ending
  OP_RETURN,
} OPCode;

/**
 * Struct to hold bytecode arr for the instructions given by some lines
 */
typedef struct {
  int count;
  int capacity;
  uint8_t* bytes;
  int line_capacity;
  int line_count;
  int* lines;
  int* instruction_counts;
  ValueArray constants;
} Code;

void init_code(Code* code);
void write_code(Code* code, uint8_t byte, int line);
void write_constant(Code* code, Value val, int line);
void free_code(Code* code);  // frees memory
int add_constant(Code* code, Value val);
int get_line(Code* code, int index);

#endif