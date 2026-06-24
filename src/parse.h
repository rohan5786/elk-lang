#ifndef PARSE_H
#define PARSE_H

#include "vm.h"
#include "lex.h"
#include "table.h"

#include <stdbool.h>

struct VarTable;

typedef struct Parser {
    Token cur;
    Token prev;
    bool error;

    struct VarTable* cur_scope;
    int local_vm_count; // tokens
} Parser;

void init_parser();
bool compile(const char *source, Code *code);

#endif