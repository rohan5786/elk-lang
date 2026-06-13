#include "parse.h"
#include "lex.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <stdlib.h>

Parser parse;
Code *compiling_code;

static void arithmetic();
static void commutative();
static void negate();
static void raw_and_parentheses();

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

// error message if there is a message
static void finish(LexType type, const char *msg) {
    if (parse.cur.type == type) {
        next_token();
        return;
    }
    error_msg_logic(&parse.cur, msg);
}

static void emit_byte(uint8_t byte) {
    write_code(cur_code(), byte, parse.prev.line);
}

static void emit_num() {
    double val = strtod(parse.prev.start, NULL);
    write_constant(compiling_code, (Value) val, parse.prev.line); // typecasting ok since Value is lit a double lol
    // add_constant((Value) val, parse.prev.line);
}

static void end_compile() {
    emit_byte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    if (!parse.error) disassemble(cur_code());
#endif
}

// Priority order: numbers, parentheses, negate, mult/div, add/sub

// goes past automatically
static bool match_then_next(LexType type) {
    if (parse.cur.type != type) return false;
    next_token();
    return true;
}

// NUMBERS, PARENTHESES
static void raw_and_parentheses() {
    if (match_then_next(LEX_NUMBER)) {
        emit_num();
        return;
    }

    if (match_then_next(LEX_LEFT_PAREN)) {
        arithmetic(); // restart
        finish(LEX_RIGHT_PAREN, "Expected ')'.");
    }
}

// NEGATE -> symbol, then number in syntax (e.g. var x = - (-5);)
static void negate() {
    // int neg_count = 0;
    // while (match_then_next(LEX_MINUS)) neg_count++;
    // while loop looses count when () between - signs come in; neg_count restarts
    if (match_then_next(LEX_MINUS)) {
        negate();
        emit_byte(OP_NEGATE);
        return;
    }

    raw_and_parentheses();

    // if (!(neg_count % 2)) emit_byte(OP_NEGATE);
}

// MULT/DIV
static void commutative() {
    negate();

    while (match_then_next(LEX_STAR) || match_then_next(LEX_SLASH)) {
        LexType op = parse.prev.type; // getting what it was
        negate();
        emit_byte((op == LEX_SLASH) ? OP_DIV : OP_MULT);
    }
}

// ADD/SUB 
static void arithmetic() {
    commutative();

    // while loop essentially checks thru multiple number operations like a + b + c in a row
    while (match_then_next(LEX_PLUS) || match_then_next(LEX_MINUS)) {
        LexType op = parse.prev.type; // getting what it was
        commutative();
        emit_byte((op == LEX_MINUS) ? OP_SUB : OP_ADD);
    }
}

bool compile(const char *source, Code *code) {
    init_lexer(source);
    compiling_code = code;
    init_parser();
    next_token();
    
    while (parse.cur.type != LEX_EOF) {
        arithmetic();
        finish(LEX_SEMICOLON, "Expected ';' at the end of expression.");
    }

    // finish(LEX_EOF, "End of expression.");
    end_compile();
    return !parse.error;
}