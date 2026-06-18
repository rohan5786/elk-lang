#ifndef VECTOR_H
#define VECTOR_H

#include "value.h"

struct Value;

typedef struct {
    int capacity;
    int count;
    struct Value* items;
} Vector;

void init_vec(Vector* vec);
void vec_push(Vector* vec, Value val);
void free_vec(Vector* vec);
int veccmp(Vector* a, Vector* b);

#endif