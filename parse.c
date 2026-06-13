#include "parse.h"
#include "lex.h"

#include <stdlib.h>

Parser parse;
Code *compiling_code;

static Code *cur_code() {
    return compiling_code;
}

void init_parser() {
    parse.error = false;
    parse.mult_error = false;
}

static void error_msg_logic(Token *token, const char *msg) {
    if (parse.mult_error) return;
    parse.mult_error = true;

    fprintf(stderr, "[line %d] error ", token->line);

    if (token->type == LEX_EOF) fprintf(stderr, "at end");
    // because there's no null terminator this stupid syntax is what is necessary to print the word causing the error
    else if (token->type != LEX_ERROR) fprintf(stderr, "at \"%.*s\"", token->length, token->start);
    
    fprintf(stderr, ": %s", msg);
    parse.error = true;
}

static void next_token() {
    parse.prev = parse.cur;
    while (1) {
        parse.cur = scan_token();
        if (parse.cur.type != LEX_ERROR) break;
        error_msg_logic(&parse.cur, parse.cur.start);
    }
}

// In order to properly detect all syntax errors across a script rather than one at a time
// not useful at the moment with simple compilation being the goal
static void sync() {
    parse.mult_error = false;

    next_token();
    while (parse.cur.type != LEX_EOF) {
        if (parse.prev.type == LEX_SEMICOLON) return;
        switch (parse.cur.type) {
            case LEX_IF:
            case LEX_ELSE:
            case LEX_FOR:
            case LEX_WHILE:
            case LEX_VAR:
            case LEX_I8:
            case LEX_I16:
            case LEX_I32:
            case LEX_I64:
            case LEX_F32:
            case LEX_F64:
            case LEX_RETURN:
            case LEX_STRING:
            case LEX_VECTOR:
            case LEX_VOID:
                return;
            default:
        }
    }
}

static void finish(LexType type, const char *msg) {
    if (parse.cur.type == type) {
        next_token();
        return;
    }
    error_msg_logic(&parse.cur, msg);
}

static void write_byte(uint8_t byte) {
    write_code(cur_code(), byte, parse.prev.line);
}

static void end_compile() {
    write_byte(OP_RETURN);
}

// TODO: Figure out priority-based compilation (recursive descent?)
static void expression() {
}

static void write_number() {
    double val = strtod(parse.prev.start, NULL);
    write_constant(compiling_code, (Value) val, parse.prev.line); // typecasting ok since Value is lit a double lol
    // add_constant((Value) val, parse.prev.line);
}

static void negate() {
    LexType op_type = parse.prev.type;
    expression(); // compile it
    if (op_type == LEX_MINUS) write_byte(OP_NEGATE);
}

bool compile(const char *source, Code *code) {
    init_lexer(source);
    compiling_code = code;
    init_parser();
    next_token();
    
    expression();

    finish(LEX_EOF, "End of expression.");
    end_compile();
    
    return !parse.error;
}