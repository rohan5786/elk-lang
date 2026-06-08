#include "basic.h"
#include "code.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
    Code code;
    init_code(&code);

    for (int i = 0; i < 300; i++)
        write_constant(&code, (double) (i), 100);

    // write_code(&code, RETURN, 123);
    disassemble(&code);
    free_code(&code);

    return 0;
}