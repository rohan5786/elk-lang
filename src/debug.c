#include "debug.h"

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

int disassemble_instruction(Code* code, int index) {
  printf("OPCode: %08d ", index);

  uint8_t instruction = code->bytes[index];

  const int line = get_line(code, index);
  switch (instruction) {
    case OP_CONSTANT_LONG:
      return print_constant_long("CONSTANT_LONG", code, index, line);
    case OP_CONSTANT:
      return print_constant("CONSTANT", code, index, line);
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