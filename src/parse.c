#include "parse.h"

#include "lex.h"
#include "error.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

Parser parse;
Code* compiling_code;

static void mult_comparison();
static void single_comparison();
static void arithmetic();
static void commutative();
static void negate();
static void raw_and_parentheses();

static void statement();

static Code* cur_code() { return compiling_code; }

void init_parser() {
    parse.error = false;
    parse.mult_error = false;
}

static void next_token() {
    parse.prev = parse.cur;
    while (1) {
        parse.cur = scan_token();
        // printf("CUR TOKEN TYPE: %d\n", parse.cur.type);
        if (parse.cur.type != LEX_ERROR) break;
        lex_err(&parse);
    }
}

// error message if there is a message
static void finish(LexType type, char* msg) {
    if (parse.cur.type == type) {
        next_token();
        return;
    }

    syntax_err(&parse, msg);
}

static void emit_byte(uint8_t byte) {
    write_code(cur_code(), byte, parse.prev.line);
}

static void emit_var_name(Token name) {
    char* name_str = (char*) malloc(name.length + 1);
    memcpy(name_str, name.start, name.length + 1);
    name_str[name.length] = '\0';
    // put this in the name map
}

static void emit_num() {
    double val = strtod(parse.prev.start, NULL);
    write_constant(
        compiling_code, NUM_VAL(val), 
        parse.prev.line
    );
}

static void emit_vector(uint16_t total) {
    emit_byte(OP_VECTOR);
    // size ones (lsb, hsb)
    emit_byte(total & 0b11111111);
    emit_byte((total >> 8) & 0b11111111);
}

static void end_compile() {
    emit_byte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    if (!parse.error) disassemble(cur_code());
#endif
}

// goes past automatically
static bool match_then_next(LexType type) {
    if (parse.cur.type != type) return false;
    next_token();
    return true;
}

// for nested vectors; vector<vector<vector<i8>>>
static LexType get_vector_base_type(LexType given_type) {
    if (given_type == LEX_VECTOR) {
        finish(LEX_LESS, "<");
        next_token();
        LexType inner_given_type = parse.prev.type;
        LexType inner_base_type = get_vector_base_type(inner_given_type);
        finish(LEX_GREATER, ">");
        return inner_base_type;
    }
    return given_type;
}

// no match_then_next bc otherwise will advance past what we wanna analyze
static int check_vector_inner_type(LexType inner_type) {
    switch (inner_type) {
        case LEX_I8:
        case LEX_I16:
        case LEX_I32:
        case LEX_I64:
        case LEX_F32:
        case LEX_F64: {
            if (parse.cur.type != LEX_NUMBER) {
                compile_err(&parse, "type mismatch: expected numerical value inside vector literal");
                return 0;
            }
            break;
        }
        case LEX_STR: {
            if (parse.cur.type != LEX_STRING) {
                compile_err(&parse, "type mismatch: expected string literal inside vector literal");
                return 0;
            }
            break;
        }
        case LEX_VECTOR: {
            if (parse.cur.type != LEX_LEFT_BRACE) {
                compile_err(&parse, "type mismatch: expected nested vector literal");
                return 0;
            }
            break;
        }
    }
    return 1;
}

static void vector_literal(LexType inner_type) {
    if (match_then_next(LEX_LEFT_BRACE)) {
        uint16_t total = 0;
        if (!match_then_next(LEX_RIGHT_BRACE)) {
            do {
                // switch statement on inner_type
                if (!check_vector_inner_type(inner_type)) return;

                // get to innermost vector, then do arithmetic
                if (inner_type == LEX_VECTOR) vector_literal(get_vector_base_type(inner_type));
                else mult_comparison();

                total++;
                if (total == 65536) {
                    compile_err(&parse, "size limit of 65535 elements for type 'vector'");
                    return;
                }
            } while (match_then_next(LEX_COMMA));
            finish(LEX_RIGHT_BRACE, "}");
        }
        emit_vector(total);
    }
}

// NUMBERS, LITERALS, PARENTHESES
static void raw_and_parentheses() {
    if (match_then_next(LEX_NUMBER)) {
        emit_num();
        return;
    }

    // peek not advance
    // catching vector literals when not supposed to be 
    if (parse.cur.type == LEX_LEFT_BRACE) {
        compile_err(&parse, "expected statically typed vector destination");
        return;
    }

    if (match_then_next(LEX_LEFT_PAREN)) {
        arithmetic();  // restart
        finish(LEX_RIGHT_PAREN, ")");
    }
}

// NEGATE -> symbol, then number in syntax (e.g. var x = - (-5);)
static void negate() {
    if (match_then_next(LEX_MINUS)) {
        negate();
        emit_byte(OP_NEGATE);
        return;
    }
    raw_and_parentheses();
}

// MULT/DIV
static void commutative() {
    negate();

    while (match_then_next(LEX_STAR) || match_then_next(LEX_SLASH)) {
        LexType op = parse.prev.type;  // getting what it was
        negate();
        emit_byte(lextype_to_opcode(op));
    }
}

// ADD/SUB
static void arithmetic() {
    commutative();

    // while loop essentially checks thru multiple number operations like a + b +
    // c in a row
    while (match_then_next(LEX_PLUS) || match_then_next(LEX_MINUS)) {
        LexType op = parse.prev.type;  // getting what it was
        commutative();
        emit_byte(lextype_to_opcode(op));
    }
}

// new lowest precedence
static void single_comparison() {
    arithmetic();

    while (match_then_next(LEX_LESS) || match_then_next(LEX_LESS_EQUAL) ||
            match_then_next(LEX_BANG_EQUAL) || match_then_next(LEX_EQUAL_EQUAL) ||
            match_then_next(LEX_GREATER) || match_then_next(LEX_GREATER_EQUAL)) {
    LexType op = parse.prev.type;
    arithmetic();
    emit_byte(lextype_to_opcode(op));
    }
}

static void mult_comparison() {
    single_comparison();

    while (match_then_next(LEX_AND_AND) || match_then_next(LEX_OR_OR) ||
            match_then_next(LEX_XOR)) {
        LexType op = parse.prev.type;
        single_comparison();
        emit_byte(lextype_to_opcode(op));
    }
}

static int emit_jump(OPCode jmp_code) {
    emit_byte(jmp_code);
    emit_byte(0b11111111);  // lsb placeholder (for now)
    emit_byte(0b11111111);
    return cur_code()->count - 3;  // index of opcode
}

static void update_jump_size(int offset_index) {
    // [end byte (how many instructions skipped) - byte after 2 size bytes]
    // = (count - 1) - (offset + 2)
    // = count - offset - 3
    int total_skip = cur_code()->count - offset_index - 3;

    if (total_skip > 65535)
        compile_err(&parse, "'if' block is too large for jump intersection (maximum 65535 bytes)");

    // add the byte # to the bytecode arr; lsb then hsb
    cur_code()->bytes[offset_index + 1] =
        total_skip & 0b11111111;  // same as fetching placeholder
    cur_code()->bytes[offset_index + 2] = (total_skip >> 8) & 0b11111111;
}

// if ( 1.0 or 0.0 eval ) { ... }
static void if_statement() {
  finish(LEX_LEFT_PAREN, "(");
  mult_comparison();
  finish(LEX_RIGHT_PAREN, ")");

  int if_offset_index = emit_jump(OP_FALSE_JMP);

  finish(LEX_LEFT_BRACE, "{");
  while (parse.cur.type != LEX_RIGHT_BRACE && parse.cur.type != LEX_EOF)
    statement();
  finish(LEX_RIGHT_BRACE, "}");

  // handle else case after
  if (match_then_next(LEX_ELSE)) {
    int else_offset_index = emit_jump(OP_JMP);

    update_jump_size(if_offset_index);  // jumps the OP_JMP code as well!

    if (match_then_next(LEX_IF)) {
      if_statement();
      return;
    }
    finish(LEX_LEFT_BRACE, "{");
    while (parse.cur.type != LEX_RIGHT_BRACE && parse.cur.type != LEX_EOF)
      statement();
    finish(LEX_RIGHT_BRACE, "}");

    update_jump_size(else_offset_index);
  } else {
    update_jump_size(if_offset_index);  // reg jump
  }
}

// including LEX_VAR
static int match_datatype_peek() {
    for (int i = 34; i < 44; i++)
        if (parse.cur.type == i) return 1;
    return 0;
}

// [35, 42] for lextype enum (no LEX_VAR)
static int match_datatype_then_next() {
    for (int i = 35; i < 44; i++)
        if (match_then_next(i)) return 1;
    return 0;
}

// vector<...> name = {...};
static void parse_vector() {
    next_token(); // consume
    // syntax checks
    if (!match_then_next(LEX_LESS)) {
        syntax_err(&parse, "<");
        return;
    }
    if (!match_datatype_then_next()) {
        compile_err(&parse, "expected statically typed inner datatype for type 'vector'");
        return;
    }

    LexType base_type = get_vector_base_type(parse.prev.type);

    if (!match_then_next(LEX_GREATER)) {
        syntax_err(&parse, ">");
        return;
    }
    if (!match_then_next(LEX_IDENTIFIER)) {
        compile_err(&parse, "expected variable name identifier after type declaration");
        return;
    }
    // everything well so far
    const Token var_name = parse.prev; // lex_identifier

    // raw_and_parentheses() creates it
    if (match_then_next(LEX_EQUAL)) {
        vector_literal(base_type);
    } else {
        emit_vector(0); // size 0
    }

    finish(LEX_SEMICOLON, ";");
    emit_var_name(var_name);
}

// TODO: finish datatype parsing
// static void parse_int(uint8_t bits) {
//     next_token(); // consume
//     if (!match_then_next(LEX_IDENTIFIER)) {
//         compile_err(&parse, "expected variable name identifier after type declaration");
//         return;
//     }

//     const Token var_name = parse.prev; 
    
//     // emit_byte(OP_I??);

//     if (match_then_next(LEX_EQUAL)) {
//         const int64_t value = strtoll(parse.prev.start, NULL, 10); // base 10 lol
//         switch (bits) {
//             case 8: {
//                 int8_t newval = (int8_t) value;
//                 // emit_byte()
//             }
//             case 16: return (int16_t) value;
//             case 32: return (int32_t) value;
//             case 64: return (int64_t) value;
//         }
//     } else {
        
//     }

//     finish(LEX_SEMICOLON, ";");
//     emit_var_name(var_name);
// }

// static void parse_float(uint8_t bits) {
// }

// DATATYPE RECOGNITION
static void var_declaration() {
    LexType var_type = parse.cur.type;

    switch (var_type) {
        case LEX_VECTOR: {
            parse_vector();
            break;
        }
        case LEX_I8: {
            // parse_int(8);
            break;
        }
        case LEX_I16:{
            // parse_int(16);
            break;
        }
        case LEX_I32: {
            // parse_int(32);
            break;
        }
        case LEX_I64: {
            // parse_int(64);
            break;
        }
        case LEX_F32: {
            break;
        }
        case LEX_F64: {
            break;
        }
        case LEX_VAR: {
            break;
        }
    }
}


// TOOD: implement var name mapping
// ---> after mapping, implement datatype syntax parsing
// ---> after parsing datatypes, many large scale tests 

static void statement() {
    if (match_datatype_peek()) 
        var_declaration();
    else if (match_then_next(LEX_IF)) 
        if_statement();
    else {
        mult_comparison();
        finish(LEX_SEMICOLON, ";");
    }
}

bool compile(const char* source, Code* code) {
  init_lexer(source);
  compiling_code = code;
  init_parser();
  next_token();

  while (parse.cur.type != LEX_EOF) statement();

  end_compile();
  return !parse.error;
}

// static void error_msg_logic(Token* token, const char* msg) {
//   if (parse.mult_error) return;
//   parse.mult_error = true;

//   fprintf(stderr, "[line %d] error ", token->line);

//   if (token->type == LEX_EOF) fprintf(stderr, "at end");
//   // because there's no null terminator this stupid syntax is what is necessary
//   // to print the word causing the error
//   else if (token->type != LEX_ERROR)
//     fprintf(stderr, "at \"%.*s\"", token->length, token->start);

//   fprintf(stderr, ": %s", msg);
//   parse.error = true;
// }