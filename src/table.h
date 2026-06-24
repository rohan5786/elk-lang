#ifndef TABLE_H
#define TABLE_H

#include "parse.h"

#include <stdint.h>

typedef struct Parser Parser;

typedef enum {
    H_INT,
    H_FLOAT,
    H_STR,
    H_VECTOR,
} VarType;

typedef struct {
    const char* key;
    int length;
    VarType type;
    int bit_num; // if int
    int vm_stack_slot;
} VarEntry;

// classic arr
typedef struct {
    VarEntry* entries;
    int capacity;
    int count;
} VarTableLayer;

typedef struct VarTable {
    VarTableLayer layers[3]; // google.com says 3 is good
    struct VarTable* parent_scope;
} VarTable;

static uint32_t hash(const char* key, int key_len, int tier_index);
VarTable* init_table(VarTable* parent_table);
int insert_table(VarTable* table, VarType type, int bit_num, const char* key, int length, int vm_slot);
VarEntry* lookup_table(VarTable* table, const char* key, int length);
void free_scope(Parser* p);

#endif