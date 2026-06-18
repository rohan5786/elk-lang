#include "vector.h"
#include "memory.h"

// TODO: write rest of vector functions
void init_vec(Vector* vec) {
    vec->capacity = 0;
    vec->count = 0;
    vec->items = NULL;
}

// classic
void vec_push(Vector* vec, Value val) {
    if (vec->capacity < vec->count + 1) {
        int old_cap = vec->capacity;
        vec->capacity = ADD_CAPACITY(old_cap);
        vec->items = ADD_ARR(Value, vec->items, vec->capacity);
    }
    vec->items[vec->count] = val;
    vec->count++;
}

void free_vec(Vector* vec) {
    ADD_ARR(Value, vec->items, 0); // free the items
    free(vec); // free the controller
}

int veccmp(Vector* a, Vector* b) {
    if (a->count != b->count) return 0;
    for (int i = 0; i < a->count; i++)
        if (!values_equal(a->items[i], b->items[i])) return 0;
    return 1;
}