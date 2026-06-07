#include "debug.h"
#include "value.h"

void disassemble(Code *code)
{
    // printf("----------------TEST CODE----------------\n");
    for (int index = 0; index < code->count;)
        index = disassemble_instruction(code, index, get_line(code, index));
}

int disassemble_instruction(Code *code, int index, int line)
{
    printf("Code: %08d ", index);

    uint8_t instruction = code->bytes[index];

    switch (instruction)
    {
    case CONSTANT:
        return print_constant("CONSTANT", code, index, line);
    case RETURN:
        return print_instruction("RETURN", index, line);
    default:
        printf("Unknown code\n");
        return index + 1;
    }
}

int print_instruction(const char *to_print, int index, int line)
{
    printf("%s    %d\n", to_print, line);
    return index + 1;
}

int print_constant(const char *to_print, Code *code, int index, int line)
{
    uint8_t constant_index = code->bytes[index + 1];
    printf("%s %d", to_print, constant_index);
    print_value(code->constants.values[constant_index]);
    printf("    %d\n", line);

    return index + 2; // first byte is the OPCode, so you want to skip the index w/the constant
}