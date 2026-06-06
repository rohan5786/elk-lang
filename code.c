#include "code.h"
#include "memory.h"

void init(Code *code) {
    (*code).capacity = 0;
    (*code).count = 0;
    (*code).code = NULL;
}

void add(Code *code, uint8_t new_byte) {
    // not enough space for one more byte, double space currently
    if ((*code).capacity < (*code).count + 1) {
        int old_capacity = (*code).capacity;
        (*code).capacity = ADD_CAPACITY(old_capacity);
        // (*code).code = ADD_CODE()
    }

    (*code).code[(*code).count] = new_byte;
    (*code).count++;
}