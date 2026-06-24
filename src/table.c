#include "table.h"

#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "error.h"

#define SEED_0 1156
#define SEED_1 5083
#define SEED_2 7461

#define FNV_OFFSET_BASIS 2166136261u
#define FNV_PRIME 16777619u

// fnv-1a for speed alone
static uint32_t hash(const char* key, int length, int tier_index) {
    uint32_t hash = FNV_OFFSET_BASIS;
    hash += (tier_index < 1) ? SEED_0 : ((tier_index < 2) ? SEED_1 : SEED_2);
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= FNV_PRIME;
    }
    return hash;
}

VarTable* init_table(VarTable* parent_table) {
    // malloc and memory and stuff
    VarTable* table = malloc(sizeof(VarTable));
    table->parent_scope = parent_table;
    // l0 = 64, l1 = 32, l2 = 16 ideally more than enough
    for (int i = 6; i > 3; i--) {
        table->layers[6 - i].capacity = (1 << i);
        table->layers[6 - i].count = 0;
        table->layers[6 - i].entries = calloc((1 << i), sizeof(VarEntry)); // capacity
    }
    return table;
}

// -1 = no space, 0 = all good, 1 = redeclaration err
int insert_table(VarTable* table, VarType type, int bit_num, const char* key, int length, int vm_slot) {
    // go thru layers
    for (int i = 0; i < 3; i++) {
        VarTableLayer* cur_layer = &table->layers[i];
        // peep 70% capacity = skip on l[0|1]
        if (cur_layer->count > cur_layer->capacity * 0.7 && i < 2) continue;

        const uint32_t hash_val = hash(key, length, i);
        uint32_t cur_index = hash_val % cur_layer->capacity;

        int probes = 0;
        while (cur_layer->entries[cur_index].key != NULL) {
            // tryna redeclare, memcmp to set certain length
            VarEntry* cur_entry = &cur_layer->entries[cur_index];
            if (cur_entry->length == length && !memcmp(cur_entry->key, key, length)) return 1;

            if (++probes > 4 && i < 2) break;

            cur_index = (cur_index + 1) % cur_layer->capacity;
        }

        if (cur_layer->entries[cur_index].key != NULL && i < 2) continue; // probe limit
        
        char* saved_key = malloc(length + 1);
        memcpy(saved_key, key, length);
        saved_key[length] = '\0';

        // insert
        cur_layer->entries[cur_index].key = saved_key;
        cur_layer->entries[cur_index].vm_stack_slot = vm_slot;
        cur_layer->entries[cur_index].length = length;
        cur_layer->entries[cur_index].type = type;
        cur_layer->entries[cur_index].bit_num = bit_num;
        cur_layer->count++;
        return 0;
    }
    return -1;
}

// like undo insert structure
VarEntry* lookup_table(VarTable* table, const char* key, int length) {
    VarTable* cur_scope = table;
    // from in->out, checking all scopes
    while (cur_scope != NULL) {
        for (int i = 0; i < 3; i++) {
            VarTableLayer* cur_layer = &cur_scope->layers[i];

            uint32_t hash_val = hash(key, length, i);
            uint32_t cur_index = hash_val % cur_layer->capacity;

            int probes = 0;
            while (cur_layer->entries[cur_index].key != NULL && probes++ < cur_layer->capacity) {
                VarEntry* cur_entry = &cur_layer->entries[cur_index];
                if (cur_entry->length == length && !memcmp(cur_entry->key, key, length)) return cur_entry;
                cur_index = (cur_index + 1) % cur_layer->capacity;
            }
        }
        cur_scope = cur_scope->parent_scope;
    }
    return NULL;
}

void free_scope(Parser* p) {
    VarTable* to_free = p->cur_scope;
    p->cur_scope = p->cur_scope->parent_scope; // go back a scope

    // free everything except our now cur_scope; to_free->parent_scope
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < to_free->layers[i].capacity; j++) {
            if (to_free->layers[i].entries[j].key != NULL) {
                free((void*) to_free->layers[i].entries[j].key);
            }
        }
        free((void*) to_free->layers[i].entries);
    }
    free((void*) to_free);
}