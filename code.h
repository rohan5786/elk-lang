#ifndef ELK_LANG_CODE_H
#define ELK_LANG_CODE_H

#include "basic.h"

/**
 * In bytecode format, each instruction must have a one-byte operation code (e.g. add/subtract)
 * Defining th eones we want for now
 */
typedef enum
{
    RETURN,
} OPCode;

/**
 * This is a struct to hold the address of the "code" for any operation (1 byte --> an unsigned 8-bit integer)
 * everything (count and capacity) is in bytes
 */
typedef struct
{
    int count;
    int capacity;
    uint8_t *code;
} Code;

void init(Code *code);
void add(Code *code, uint8_t byte);
void free_code(Code *code); // frees memory

#endif