#ifndef PARSE_H
#define PARSE_H

#include "vm.h"
#include "lex.h"
#include <stdbool.h>

typedef struct {
    Token cur;
    Token prev;
    bool mult_error;
    bool error;
} Parser;

void init_parser();
bool compile(const char *source, Code *code);

#endif