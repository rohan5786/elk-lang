#ifndef ERROR_H
#define ERROR_H

#include "lex.h"
#include "parse.h"

#include <stdlib.h>

void lex_err(Parser* p);
void syntax_err(Parser* p, char* expected);
void runtime_err(Value cur_val, const char* msg);
void compile_err(Parser* p, const char* msg);
void compile_warn(Parser* p, const char* msg);

#endif