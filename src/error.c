#include "error.h"
#include "parse.h"

#include <stdlib.h>
#include <stdio.h>

void lex_err(Parser* p) {
    if (p->error) p->mult_error = true;

    fprintf(stderr, "[line %d] lex error: %s\n", p->cur.line, p->cur.start);
    
    p->error = true;
}

void syntax_err(Parser* p, char* expected) {
    if (p->error) p->mult_error = true;
    
    fprintf(stderr, "[line %d] syntax error: expected '%s'\n", p->cur.line, expected);

    p->error = true;
}

void runtime_err(Value cur_val, const char* msg) {
    fprintf(stderr, "[at Value(s): ");
    print_err_value(cur_val);
    fprintf(stderr, "]\nruntime error: %s\n", msg);
}

void compile_err(Parser* p, const char* msg) {
    if (p->error) p->mult_error = true;

    fprintf(stderr, "[line %d] compile-time error: %s\n", p->cur.line, msg);

    p->error = true;
}

