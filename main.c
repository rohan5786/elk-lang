#include "basic.h"
#include "code.h"
#include "debug.h"

int main(int argc, const char *argv[])
{
    Code code;
    init(&code);
    add(&code, RETURN);
    disassemble(&code);
    free_code(&code);
    
    return 0;
}