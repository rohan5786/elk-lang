#include "debug.h"

void disassemble(Code *code)
{
    // printf("----------------TEST CODE----------------\n");
    for (int index = 0; index < (*code).count;) {
        index = disassemble_instruction(code, index);
    }
}

int disassemble_instruction(Code *code, int index)
{
    printf("Index: %08d\n", index);

    uint8_t instruction = (*code).code[index];

    switch (instruction) {
        case RETURN:
            return print_instruction("RETURN", index);
        default:
            printf("Unknown code\n");
            return index + 1;
    }
}

int print_instruction(const char *to_print, int index) {
    printf("CODE: %s\n", to_print);
    return index + 1;
}