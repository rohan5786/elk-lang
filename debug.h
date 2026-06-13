#ifndef DEBUG_H
#define DEBUG_H
#define DEBUG_TRACE

#include "code.h"

// disassemble all instructions
void disassemble(Code *code);
int disassemble_instruction(Code *code, int index);
int print_instruction(const char *to_print, int index, int line);
int print_constant(const char *to_print, Code *code, int index, int line);
int print_constant_long(const char *to_print, Code *code, int index, int line);

#endif