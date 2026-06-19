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
void vec_push(Vector* vec, struct Value val);
struct Value vec_get(Vector* vec, int index);
void free_vec(Vector* vec);
int veccmp(Vector* a, Vector* b);

// add sort later lol not tryna add this for elk 1.0

#endif