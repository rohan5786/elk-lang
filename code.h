#ifndef ELK_LANG_CODE_H
#define ELK_LANG_CODE_H

#include "basic.h"
#include "value.h"

/**
 * In bytecode format, each instruction must have a one-byte operation code (e.g. add/subtract)
 * Defining th eones we want for now
 */
typedef enum
{
    CONSTANT,
    RETURN,
} OPCode;

/**
 * Struct to hold bytecode arr for the instructions given by some lines
 */
typedef struct
{
    int count;
    int capacity;
    uint8_t *bytes;
    int line_capacity;
    int line_count;
    int *lines;
    int *instruction_counts;
    ValueArray constants;
} Code;

void init_code(Code *code);
void write_code(Code *code, uint8_t byte, int line);
void free_code(Code *code); // frees memory
int add_constant(Code *code, Value val);
int get_line(Code *code, int index);

#endif