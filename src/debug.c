#include "debug.h"

#include <stdio.h>

#include "value.h"

void disassemble(Code* code) {
  // printf("----------------TEST CODE----------------\n");
  for (int index = 0; index < code->count;)
    index = disassemble_instruction(code, index);
}

int jmp_instruction(const char *to_print, int index, int line) {
  printf("%s    %d\n", to_print, line);
  return index + 3;
}

int print_instruction(const char* to_print, int index, int line) {
  printf("%s    %d\n", to_print, line);
  return index + 1;
}

int print_constant(const char* to_print, Code* code, int index, int line) {
  uint8_t const_index = code->bytes[index + 1];
  printf("%s \t Index: %d \t ", to_print, const_index);
  print_value(code->constants.values[const_index]);
  printf(" \t Line: %d\n", line);

  return index + 2;  // first byte is the OPCode, so you want to skip the index
                     // w/the constant
}

int print_constant_long(const char* to_print, Code* code, int index, int line) {
  uint8_t byte1 = code->bytes[index + 1];  // LSB
  uint8_t byte2 = code->bytes[index + 2];
  uint8_t byte3 = code->bytes[index + 3];  // MSB

  int value_full = byte1 | (byte2 << 8) | (byte3 << 16);

  printf("%s \t Index: %d \t ", to_print, value_full);
  print_value(code->constants.values[value_full]);
  printf(" \t Line: %d\n", line);
  return index + 4;
}

// TODO: FIX THIS 
int print_vector(const char* to_print, int index, int line) {
  printf("%s    %d\n", to_print, line);
  return index + 3;
}

int disassemble_instruction(Code* code, int index) {
  printf("OPCode: %08d ", index);

  uint8_t instruction = code->bytes[index];

  const int line = get_line(code, index);
  switch (instruction) {
    case OP_GET_LOCAL: {
      print_instruction("GET_LOCAL", index, line); 
      return index + 3;
    }
    case OP_SET_LOCAL: {
      print_instruction("SET_LOCAL", index, line);
      return index + 3;
    }
    case OP_VECTOR:
      return print_vector("VECTOR", index, line);
    case OP_I8: return index + 2;
      // return print_constant("I8", code, index, line);
    case OP_I16: return index + 3;
      // return print_constant("I16", code, index, line);
    case OP_I32: return index + 5;
      // return print_constant("I32", code, index, line);
    case OP_I64: return index + 9;
      // return print_constant("I64", code, index, line);
    case OP_F32: return index + 5;
      // return print_constant("F32", code, index, line);
    case OP_F64: return index + 9;
      // return print_constant("F64", code, index, line);
    case OP_STR:
      // return wtf
    case OP_ADD:
      return print_instruction("ADD", index, line);
    case OP_SUB:
      return print_instruction("SUBTRACT", index, line);
    case OP_MULT:
      return print_instruction("MULT", index, line);
    case OP_DIV:
      return print_instruction("DIV", index, line);
    case OP_NEGATE:
      return print_instruction("NEGATE", index, line);
    case OP_LESS:
      return print_instruction("LESS", index, line);
    case OP_EQUAL:
      return print_instruction("EQUAL", index, line);
    case OP_GREATER:
      return print_instruction("GREATER", index, line);
    case OP_AND:
      return print_instruction("AND", index, line);
    case OP_OR:
      return print_instruction("OR", index, line);
    case OP_XOR:
      return print_instruction("XOR", index, line);
    case OP_JMP:
      return jmp_instruction("JMP", index, line);
    case OP_TRUE_JMP:
      return jmp_instruction("TRUE_JMP", index, line);
    case OP_FALSE_JMP:
      return jmp_instruction("FALSE_JMP", index, line);
    case OP_RETURN:
      return print_instruction("RETURN", index, line);
    default:
      printf("Unknown code\n");
      return index + 1;
  }
}