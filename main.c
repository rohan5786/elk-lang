#include "basic.h"
#include "code.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
    Code code;
    init_code(&code);
    
    double number = add_constant(&code, 2.0); // index
    write_code(&code, CONSTANT, 123);
    write_code(&code, number, 123);
    
    write_code(&code, RETURN, 123);
    disassemble(&code);
    free_code(&code);

    return 0;
}