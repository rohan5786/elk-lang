#include "parse.h"

#include "error.h"
#include "lex.h"
#include "table.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Parser parse;
Code* compiling_code;

static void emit_int(const bool has_equals, int bits, int64_t value);
static void emit_float(int bits, double value);

static void mult_comparison(int bit_num);
static void single_comparison(int bit_num);
static void arithmetic(int bit_num);
static void commutative(int bit_num);
static void negate(int bit_num);
static void raw_and_parentheses(int bit_num);

static void statement();

static Code* cur_code() { return compiling_code; }

void init_parser() {
  parse.error = false;
  parse.cur_scope = init_table(NULL);
  parse.local_vm_count = 0;
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
  next_token(); // DEBUG
}

static void emit_byte(uint8_t byte) {
  write_code(cur_code(), byte, parse.prev.line);
}

static void emit_num(const bool zilch, int bit_num) {
  switch (bit_num) {
    case 0:
    case 1: {
      const double value = zilch ? 0 : strtod(parse.prev.start, NULL);
      emit_float(bit_num, value);
      break;
    }
    case 8:
    case 16:
    case 32:
    case 64: {
      const int64_t value = zilch ? 0 : strtod(parse.prev.start, NULL);
      emit_int(true, bit_num, (int64_t)(value));
      break;
    }
    default: {
      const double value = zilch ? 0 : strtod(parse.prev.start, NULL);
      emit_float(0, value);
    }
  }
  // FOR NOW; TEMP
  // write_constant(
  //     compiling_code, NUM_VAL(val),
  //     parse.prev.line
  // );
}

static void emit_vector(uint16_t total) {
  emit_byte(OP_VECTOR);
  // size ones (lsb, hsb)
  emit_byte(total & 0b11111111);
  emit_byte((total >> 8) & 0b11111111);
}

static void emit_str(int length) {
    emit_byte(OP_STR);
    for (int i = 0; i < 32; i += 8)
        emit_byte((length >> i) & 0b11111111);
    for (int i = 0; i < length; i++)
        emit_byte((uint8_t) parse.prev.start[i]); // unsigned!
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
        compile_err(
            &parse,
            "type mismatch: expected numerical value inside vector literal");
        return 0;
      }
      break;
    }
    case LEX_STR: {
      if (parse.cur.type != LEX_STRING) {
        compile_err(&parse,
                    "type mismatch: expected type 'str' inside vector literal");
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
        if (match_then_next(LEX_COMMA)) {
          // replacing "_" w/datatype name
          const char* datatype = lex_datatype_to_str(inner_type);
          char compile_err_msg[] =
              "type mismatch: expected type '_' inside vector literal";
          char* sub_pos = strstr(compile_err_msg, "_");
          char* after_sub_pos = sub_pos + strlen("_");

          // moving tail part down to be [sub_pos + strlen(datatype)] units
          // after
          memmove(sub_pos + strlen(datatype), after_sub_pos,
                  strlen(after_sub_pos) + 1);  // + 1 = '\0'
          // copying datatype str into new mem space
          memcpy(sub_pos, datatype, strlen(datatype));

          compile_err(&parse, compile_err_msg);
          break;
        }

        // switch statement on inner_type
        if (!check_vector_inner_type(inner_type)) break;

        // get to innermost vector, then do arithmetic
        if (inner_type == LEX_VECTOR)
          vector_literal(get_vector_base_type(inner_type));
        else {
          switch (inner_type) {
            case LEX_I8: {
              mult_comparison(8);
              break;
            }
            case LEX_I16: {
              mult_comparison(16);
              break;
            }
            case LEX_I32: {
              mult_comparison(32);
              break;
            }
            case LEX_I64: {
              mult_comparison(64);
              break;
            }
            case LEX_F32: {
              mult_comparison(0);
              break;
            }
            case LEX_F64: {
              mult_comparison(1);
              break;
            }
            default: {
              mult_comparison(-1);
              break;
            }
          }
        }

        total++;
        if (total > 65535) {
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
// -1 = default to f32, 0 = f32, 1 = f64, 8 = i8, 16 = i16, 32 = i32, 64 = i64
static void raw_and_parentheses(int bit_num) {
  if (match_then_next(LEX_NUMBER) ^ match_then_next(LEX_ZILCH)) {
    emit_num(parse.prev.type == LEX_ZILCH, bit_num);
    return;
  }

  if (match_then_next(LEX_IDENTIFIER)) {
    const Token var_token = parse.prev;
    const VarEntry* entry = lookup_table(parse.cur_scope, var_token.start, var_token.length);

    if (entry == NULL) {
        compile_err(&parse, "undefined variable identifier name in this scope");
        return;
    }

    emit_byte(OP_GET_LOCAL);
    emit_byte((uint8_t)(entry->vm_stack_slot & 0b11111111));
    emit_byte((uint8_t)((entry->vm_stack_slot >> 8) & 0b11111111));
    return;
  }

  if (match_then_next(LEX_STRING)) {
    emit_str(parse.prev.length);
    return;
  }

  // peek not advance
  // catching vector literals when not supposed to be
  if (parse.cur.type == LEX_LEFT_BRACE) {
    compile_err(&parse, "expected statically typed vector destination");
    return;
  }

  if (match_then_next(LEX_LEFT_PAREN)) {
    arithmetic(bit_num);  // restart
    finish(LEX_RIGHT_PAREN, ")");
  }
}

// NEGATE -> symbol, then number in syntax (e.g. var x = - (-5);)
static void negate(int bit_num) {
  if (match_then_next(LEX_MINUS)) {
    negate(bit_num);
    emit_byte(OP_NEGATE);
    return;
  }
  raw_and_parentheses(bit_num);
}

// MULT/DIV
static void commutative(int bit_num) {
  negate(bit_num);

  while (match_then_next(LEX_STAR) || match_then_next(LEX_SLASH)) {
    LexType op = parse.prev.type;  // getting what it was
    negate(bit_num);
    emit_byte(lextype_to_opcode(op));
  }
}

// ADD/SUB
static void arithmetic(int bit_num) {
  commutative(bit_num);

  // while loop essentially checks thru multiple number operations like a + b +
  // c in a row
  while (match_then_next(LEX_PLUS) || match_then_next(LEX_MINUS)) {
    LexType op = parse.prev.type;  // getting what it was
    commutative(bit_num);
    emit_byte(lextype_to_opcode(op));
  }
}

// new lowest precedence
static void single_comparison(int bit_num) {
  arithmetic(bit_num);

  while (match_then_next(LEX_LESS) || match_then_next(LEX_LESS_EQUAL) ||
         match_then_next(LEX_BANG_EQUAL) || match_then_next(LEX_EQUAL_EQUAL) ||
         match_then_next(LEX_GREATER) || match_then_next(LEX_GREATER_EQUAL)) {
    LexType op = parse.prev.type;
    arithmetic(bit_num);
    emit_byte(lextype_to_opcode(op));
  }
}

static void mult_comparison(int bit_num) {
  single_comparison(bit_num);

  while (match_then_next(LEX_AND_AND) || match_then_next(LEX_OR_OR) ||
         match_then_next(LEX_XOR)) {
    LexType op = parse.prev.type;
    single_comparison(bit_num);
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
    compile_err(
        &parse,
        "'if' block is too large for jump intersection (maximum 65535 bytes)");

  // add the byte # to the bytecode arr; lsb then hsb
  cur_code()->bytes[offset_index + 1] =
      total_skip & 0b11111111;  // same as fetching placeholder
  cur_code()->bytes[offset_index + 2] = (total_skip >> 8) & 0b11111111;
}

// if ( 1.0 or 0.0 eval ) { ... }
static void if_statement() {
  finish(LEX_LEFT_PAREN, "(");
  mult_comparison(-1);
  finish(LEX_RIGHT_PAREN, ")");

  int if_offset_index = emit_jump(OP_FALSE_JMP);

  // NEW SCOPE
  finish(LEX_LEFT_BRACE, "{");
  parse.cur_scope = init_table(parse.cur_scope);

  while (parse.cur.type != LEX_RIGHT_BRACE && parse.cur.type != LEX_EOF)
    statement();

  finish(LEX_RIGHT_BRACE, "}");
  free_scope(&parse);
  // CLOSE SCOPE

  // handle else case after
  if (match_then_next(LEX_ELSE)) {
    int else_offset_index = emit_jump(OP_JMP);

    update_jump_size(if_offset_index);  // jumps the OP_JMP code as well!

    if (match_then_next(LEX_IF)) {
      if_statement();
      return;
    }

    // NEW SCOPE
    finish(LEX_LEFT_BRACE, "{");
    parse.cur_scope = init_table(parse.cur_scope);

    while (parse.cur.type != LEX_RIGHT_BRACE && parse.cur.type != LEX_EOF)
      statement();

    finish(LEX_RIGHT_BRACE, "}");
    free_scope(&parse);
    // CLOSE SCOPE

    update_jump_size(else_offset_index);
  } else {
    update_jump_size(if_offset_index);  // reg jump
  }
}

// including LEX_VAR
static int match_datatype_peek() {
  for (int i = 37; i < 46; i++)
    if (parse.cur.type == i) return 1;
  return 0;
}

// [38, 45] for lextype enum (no LEX_VAR)
static int match_datatype_then_next() {
  for (int i = 38; i < 46; i++)
    if (match_then_next(i)) return 1;
  return 0;
}

// vector<...> name = {...};
static void parse_vector() {
  next_token();  // consume
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
  const Token var_name = parse.prev;  // lex_identifier

  // raw_and_parentheses() creates it
  if (match_then_next(LEX_EQUAL)) {
    vector_literal(base_type);
  } else {
    emit_vector(0);  // size 0
  }
  finish(LEX_SEMICOLON, ";");

  // 0 default bit num
  int result = insert_table(parse.cur_scope, H_VECTOR, -1, var_name.start, var_name.length, parse.local_vm_count);

  if (result < 0) {
    compile_err(&parse, "vartable full");
    return;
  } else if (result > 0) {
    char* str = malloc(var_name.length + 1);
    memcpy(str, var_name.start, var_name.length + 1);
    str[var_name.length] = '\0';
    // do what compile_err does here
    fprintf(stderr,
            "[line %d] compile-time error: redeclaration of variable '%s' in "
            "this scope",
            parse.cur.line, str);
    parse.error = true;
    return;
  }
  parse.local_vm_count++;
}

static void emit_int(const bool has_equals, int bits, int64_t value) {
  switch (bits) {
    case 8: {
      int8_t newval = (int8_t)value;
      emit_byte(OP_I8);
      if (!has_equals) {
        emit_byte(0);
        return;
      }
      emit_byte(newval);
      break;
    }
    case 16: {
      int16_t newval = (int16_t)value;
      emit_byte(OP_I16);
      if (!has_equals) {
        emit_byte(0);
        return;
      }
      emit_byte(newval & 0b11111111);
      emit_byte((newval >> 8) & 0b11111111);
      break;
    }
    case 32: {
      int32_t newval = (int32_t)value;
      emit_byte(OP_I32);
      if (!has_equals) {
        emit_byte(0);
        return;
      }
      emit_byte(newval & 0b11111111);
      emit_byte((newval >> 8) & 0b11111111);
      emit_byte((newval >> 16) & 0b11111111);
      emit_byte((newval >> 24) & 0b11111111);
      break;
    }
    case 64: {
      int64_t newval = (int64_t)value;
      emit_byte(OP_I64);
      if (!has_equals) {
        emit_byte(0);
        return;
      }
      emit_byte(newval & 0b11111111);
      emit_byte((newval >> 8) & 0b11111111);
      emit_byte((newval >> 16) & 0b11111111);
      emit_byte((newval >> 24) & 0b11111111);
      emit_byte((newval >> 32) & 0b11111111);
      emit_byte((newval >> 40) & 0b11111111);
      emit_byte((newval >> 48) & 0b11111111);
      emit_byte((newval >> 56) & 0b11111111);
      break;
    }
  }
}

// HAS i64 compatibility
static void parse_int(uint8_t bits) {
  next_token();  // consume type

  if (!match_then_next(LEX_IDENTIFIER)) {
    compile_err(&parse,
                "expected variable name identifier after type declaration");
    return;
  }

  const Token var_name = parse.prev;
  const bool has_equals = match_then_next(LEX_EQUAL);
  int64_t value = 0;

  if (has_equals) {
    if (parse.cur.type != LEX_NUMBER && parse.cur.type != LEX_ZILCH) {
      compile_err(&parse, "expected integer value after '='");
      return;
    }
    mult_comparison(bits);
    // next_token(); // moves to ;
    // value = strtoll(parse.prev.start, NULL, 10); // base 10 lol
  }
  finish(LEX_SEMICOLON, ";");

  int result = insert_table(parse.cur_scope, H_INT, bits, var_name.start,
                            var_name.length, parse.local_vm_count);

  if (result < 0) {
    compile_err(&parse, "vartable full");
    return;
  } else if (result > 0) {
    char* str = malloc(var_name.length + 1);
    memcpy(str, var_name.start, var_name.length + 1);
    str[var_name.length] = '\0';
    // do what compile_err does here
    fprintf(stderr,
            "[line %d] compile-time error: redeclaration of variable '%s' in "
            "local scope",
            parse.cur.line, str);
    parse.error = true;
    return;
  }
  parse.local_vm_count++;
}

// f32 is 0 default
static void emit_float(int bit_num, double value) {
  if (!bit_num) {
    float fval = (float)value;
    union {
      float val;
      uint32_t bits;
    } cast;
    cast.val = fval;
    emit_byte(OP_F32);
    for (int i = 0; i < 32; i += 8) emit_byte((cast.bits >> i) & 0b11111111);
  } else {
    union {
      double val;
      uint64_t bits;
    } cast;
    cast.val = value;
    emit_byte(OP_F64);
    for (int i = 0; i < 64; i += 8) emit_byte((cast.bits >> i) & 0b11111111);
  }
}

static void parse_float(uint8_t bits) {
  next_token();  // consume type

  if (!match_then_next(LEX_IDENTIFIER)) {
    compile_err(&parse,
                "expected variable name identifier after type declaration");
    return;
  }

  const Token var_name = parse.prev;

  if (match_then_next(LEX_EQUAL)) {
    if (parse.cur.type != LEX_NUMBER && parse.cur.type != LEX_ZILCH) {
      compile_err(&parse, "expected floating point value after '='");
      return;
    }
    mult_comparison(bits);
    // next_token();
    // const double value = strtod(parse.prev.start, NULL);
    // emit_float((bits != 32), value);
  } else
    mult_comparison(-1);
  finish(LEX_SEMICOLON, ";");

  int result = insert_table(parse.cur_scope, H_FLOAT, bits, var_name.start,
                            var_name.length, parse.local_vm_count);

  if (result < 0) {
    compile_err(&parse, "vartable full");
    return;
  } else if (result > 0) {
    char* str = malloc(var_name.length + 1);
    memcpy(str, var_name.start, var_name.length + 1);
    str[var_name.length] = '\0';
    // do what compile_err does here
    fprintf(stderr,
            "[line %d] compile-time error: redeclaration of variable '%s' in "
            "local scope",
            parse.cur.line, str);
    parse.error = true;
    return;
  }
  parse.local_vm_count++;
}

static void parse_str() {
  next_token();  // consume type

  if (!match_then_next(LEX_IDENTIFIER)) {
    compile_err(&parse, "expected variable name identifier after type declaration");
    return;
  }

  const Token var_name = parse.prev;

  if (match_then_next(LEX_EQUAL)) {
    if (!match_then_next(LEX_STRING)) {
        compile_err(&parse, "expected string literal after '='");
        return;
    }
    emit_str(parse.prev.length);
  } else emit_str(0);
  finish(LEX_SEMICOLON, ";");

  int result = insert_table(parse.cur_scope, H_STR, -1, var_name.start, var_name.length, parse.local_vm_count);
  if (result < 0) {
    compile_err(&parse, "vartable full");
    return;
  } else if (result > 0) {
    char* str = malloc(var_name.length + 1);
    memcpy(str, var_name.start, var_name.length + 1);
    str[var_name.length] = '\0';
    fprintf(stderr,
            "[line %d] compile-time error: redeclaration of variable '%s' in "
            "local scope",
            parse.cur.line, str);
    parse.error = true;
    return;
  }

  parse.local_vm_count++;
}

static void parse_var() {
    next_token();
    if (!match_then_next(LEX_IDENTIFIER)) {
        compile_err(&parse, "expected variable name identifier after type declaration");
        return;
    }

    const Token var_name = parse.prev;

    if (!match_then_next(LEX_EQUAL)) {
        compile_err(&parse, "variable type 'var' must have an initializer");
        return;
    }
    // probably need some sort of node system to evaluate this separately then push
    // rewrite the structure: basic descent -> finalize, return evaluated Value -> emit bytes?
    mult_comparison(-1); 
    finish(LEX_SEMICOLON, ";");

    // TODO
    const VarType var_type = H_FLOAT; 
    const int bit_num = -1;
    // basic rest of insert
    int result = insert_table(parse.cur_scope, var_type, bit_num, var_name.start, var_name.length, parse.local_vm_count);
    if (result < 0) {
        compile_err(&parse, "vartable full");
        return;
    } else if (result > 0) {
        char* str = malloc(var_name.length + 1);
        memcpy(str, var_name.start, var_name.length + 1);
        str[var_name.length] = '\0';
        fprintf(stderr,
                "[line %d] compile-time error: redeclaration of variable '%s' in "
                "local scope",
                parse.cur.line, str);
        parse.error = true;
        return;
    }

    parse.local_vm_count++;
}

// DATATYPE RECOGNITION
static void var_declaration() {
    LexType var_type = parse.cur.type;

    switch (var_type) {
        case LEX_VECTOR: {
            parse_vector();
            break;
        }
        case LEX_I8: {
            parse_int(8);
            break;
        }
        case LEX_I16: {
            parse_int(16);
            break;
        }
        case LEX_I32: {
            parse_int(32);
            break;
        }
        case LEX_I64: {
            parse_int(64);
            break;
        }
        case LEX_F32: {
            parse_float(0);
            break;
        }
        case LEX_F64: {
            parse_float(1);
            break;
        }
        case LEX_STR: {
            parse_str();
            break;
        }
        case LEX_VAR: {
            parse_var();
            break;
        }
    }
}

// assignment functions
static void var_assignment(const VarEntry* entry) {
    if (match_then_next(LEX_EQUAL)) {
        mult_comparison(entry->bit_num);

        emit_byte(OP_SET_LOCAL);
        emit_byte((uint8_t)(entry->vm_stack_slot & 0b11111111));
        emit_byte((uint8_t)((entry->vm_stack_slot >> 8) & 0b11111111));
        return;
    }
    compile_warn(&parse, "statement with no effect");
}

static void vec_or_str_assignment(const VarEntry* entry) {
    // whole new vector/str
    if (match_then_next(LEX_EQUAL)) {
        mult_comparison(-1); // (entry->bit_num)

        emit_byte(OP_SET_LOCAL);
        emit_byte((uint8_t)(entry->vm_stack_slot & 0b11111111));
        emit_byte((uint8_t)((entry->vm_stack_slot >> 8) & 0b11111111));
    } else if (match_then_next(LEX_LEFT_BRACKET)) {
        mult_comparison(-1); // index value
        
        finish(LEX_RIGHT_BRACKET, "]");
        finish(LEX_EQUAL, "=");

        mult_comparison(-1); // new set value

        emit_byte(OP_SET_INDEX);
        emit_byte((uint8_t) (entry->vm_stack_slot & 0b11111111));
        emit_byte((uint8_t) ((entry->vm_stack_slot >> 8) & 0b11111111));
    } else {
        compile_err(&parse, "expected '=' or '[' after vector/str identifier");
        return;
    }
}

static void expression() {
    if (match_then_next(LEX_IDENTIFIER)) {
        const Token var_token = parse.prev;
        const VarEntry* entry = lookup_table(parse.cur_scope, var_token.start, var_token.length);
        if (entry == NULL) {
            compile_err(&parse, "undefined variable identifier name in this scope");
            return;
        }

        if (entry->type == H_STR || entry->type == H_VECTOR) {
            vec_or_str_assignment(entry);
        }
        else {
            var_assignment(entry);
        }
    } else {
        mult_comparison(-1);
    }
}

// TODO: implement var parsing + mapping syntax analysis
// ---> also, add chars? HELL NAHHH
// ---> after fully mapping and modifying, do larger scale tests
// ---> after tests; loops, then functions!
// ---> add print basic built in function (might go down a rabbit hole)
static void statement() {
    if (match_datatype_peek())
        var_declaration();
    else if (match_then_next(LEX_IF))
        if_statement();
    else {
        expression();
        // mult_comparison(-1);
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
//   // because there's no null terminator this stupid syntax is what is
//   necessary
//   // to print the word causing the error
//   else if (token->type != LEX_ERROR)
//     fprintf(stderr, "at \"%.*s\"", token->length, token->start);

//   fprintf(stderr, ": %s", msg);
//   parse.error = true;
// }