#include "parse.h"
#include "lex.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <stdlib.h>

Parser parse;
Code *compiling_code;

static void mult_comparison();
static void single_comparison();
static void arithmetic();
static void commutative();
static void negate();
static void raw_and_parentheses();

static void statement();

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
        // printf("CUR TOKEN TYPE: %d\n", parse.cur.type);
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

// Priority order: numbers, parentheses, negate, mult/div, add/sub, comparison

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

// new lowest precedence
static void single_comparison() {
    arithmetic();

    while (
        match_then_next(LEX_LESS) || 
        match_then_next(LEX_LESS_EQUAL) || 
        match_then_next(LEX_BANG_EQUAL) || 
        match_then_next(LEX_EQUAL_EQUAL) || 
        match_then_next(LEX_GREATER) || 
        match_then_next(LEX_GREATER_EQUAL)
    ) {
        LexType op = parse.prev.type;
        arithmetic();
        switch (op) {
            case LEX_LESS_EQUAL: {
                emit_byte(OP_LESS_EQL);
                break;
            }
            case LEX_LESS: {
                emit_byte(OP_LESS);
                break;
            }
            case LEX_BANG_EQUAL: {
                emit_byte(OP_NOT_EQL);
                break;
            }
            case LEX_EQUAL_EQUAL: {
                emit_byte(OP_EQUAL);
                break;
            }
            case LEX_GREATER_EQUAL: {
                emit_byte(OP_GREATER_EQL);
                break;
            }
            case LEX_GREATER: {
                emit_byte(OP_GREATER);
                break;
            }
        }
    }
}

static void mult_comparison() {
    single_comparison();

    while (match_then_next(LEX_AND_AND) || match_then_next(LEX_OR_OR) || match_then_next(LEX_XOR)) {
        LexType op = parse.prev.type;
        single_comparison();
        switch (op) {
            case LEX_AND_AND: {
                emit_byte(OP_AND);
                break;
            }
            case LEX_OR_OR: {
                emit_byte(OP_OR);
                break;
            }
            case LEX_XOR: {
                emit_byte(OP_XOR);
                break;
            }
        }
    }
}

static int emit_jump(OPCode jmp_code) {
    emit_byte(jmp_code);
    emit_byte(0b11111111); // lsb placeholder (for now)
    emit_byte(0b11111111);
    return cur_code()->count - 3; // index of opcode
}

static void update_jump_size(int offset_index) {
    // [end byte (how many instructions skipped) - byte after 2 size bytes]
    // = (count - 1) - (offset + 2)
    // = count - offset - 3
    int total_skip = cur_code()->count - offset_index - 3;

    // uint16_t max, TODO: make runtime err logic
    if (total_skip > 65535)
        error_msg_logic(&parse.cur, "Too large of an if-statement block.");
    
    // add the byte # to the bytecode arr; lsb then hsb
    cur_code()->bytes[offset_index + 1] = total_skip & 0b11111111; // same as fetching placeholder
    cur_code()->bytes[offset_index + 2] = (total_skip >> 8) & 0b11111111;
}

// if ( 1.0 or 0.0 eval ) { ... }
static void if_statement() {
    finish(LEX_LEFT_PAREN, "Expected '(' at the beginning of expression.");
    mult_comparison();
    finish(LEX_RIGHT_PAREN, "Expected ')' at the end of expression.");

    int if_offset_index = emit_jump(OP_FALSE_JMP);

    finish(LEX_LEFT_BRACE, "Expected '{' at the beginning of expression.");
    while (parse.cur.type != LEX_RIGHT_BRACE && parse.cur.type != LEX_EOF)
        statement();
    finish(LEX_RIGHT_BRACE, "Expected '}' at the end of expression.");

    // handle else case after
    if (match_then_next(LEX_ELSE)) {
        int else_offset_index = emit_jump(OP_JMP);

        update_jump_size(if_offset_index); // jumps the OP_JMP code as well!

        finish(LEX_LEFT_BRACE, "Expected '{' at the beginning of expression.");
        while (parse.cur.type != LEX_RIGHT_BRACE && parse.cur.type != LEX_EOF)
            statement();
        finish(LEX_RIGHT_BRACE, "Expected '}' at the end of expression.");

        update_jump_size(else_offset_index);
    } else {
        update_jump_size(if_offset_index); // reg jump
    }
}

static void statement() {
    if (match_then_next(LEX_IF)) {
        if_statement();
    }
    else {
        mult_comparison();
        finish(LEX_SEMICOLON, "Expected ';' at the end of expression.");
    }
}

bool compile(const char *source, Code *code) {
    init_lexer(source);
    compiling_code = code;
    init_parser();
    next_token();
    
    while (parse.cur.type != LEX_EOF)
        statement();

    // finish(LEX_EOF, "End of expression.");
    end_compile();
    return !parse.error;
}