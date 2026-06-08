#ifndef ELK_LANG_DEBUG_H
#define ELK_LANG_DEBUG_H

#include "code.h"

// disassemble all instructions
void disassemble(Code *code);
int disassemble_instruction(Code *code, int index, int line);
int print_instruction(const char *to_print, int index, int line);
int print_constant(const char *to_print, Code *code, int index, int line);
int print_constant_long(const char *to_print, Code *code, int index, int line);

#endif