#include "basic.h"
#include "code.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char *argv[])
{
    init_vm();

    Code code;
    init_code(&code);

    write_constant(&code, 300.03, 100); // has "CONSTANT"/"CONSTANT_LONG" build in
    write_code(&code, NEGATE, 101);
    write_code(&code, RETURN, 102);

    interpret(&code);
    
    free_vm();
    free_code(&code);

    return 0;
}