#include <stdlib.h>
#include "memory.h"
#include <stddef.h>

void *reallocate(void *pointer, size_t new_size) {
    if (new_size == 0) {
        free(pointer); // freeing the space for arr
        return NULL; // safety
    }

    void *result = realloc(pointer, new_size); // pointer to new size area in memory
    if (result == NULL) exit(1); // err if no memory left
    return result;
}

// void *reallocate2(void *pointer, size_t new_size) {
//     free(pointer);
//     if (new_size == 0) return NULL;

//     void *result = malloc(new_size);
//     if (result == NULL) exit(1);
//     return result;
// }
