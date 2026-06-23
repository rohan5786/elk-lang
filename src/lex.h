#ifndef LEX_H
#define LEX_H

#include "code.h"

#include <stdbool.h>

typedef struct
{
    const char *start;
    const char *cur;
    int line; // line #
} Lexer;

typedef enum {
  // Single-character tokens.
  LEX_LEFT_PAREN, LEX_RIGHT_PAREN,
  LEX_LEFT_BRACE, LEX_RIGHT_BRACE,
  LEX_COMMA, LEX_DOT, LEX_MINUS, LEX_PLUS,
  LEX_SEMICOLON, LEX_SLASH, LEX_STAR, LEX_PERCENT,

  // One or two character tokens.
  LEX_BANG, LEX_BANG_EQUAL,
  LEX_EQUAL, LEX_EQUAL_EQUAL,
  LEX_GREATER, LEX_GREATER_EQUAL,
  LEX_LESS, LEX_LESS_EQUAL,
  LEX_AND_AND, LEX_AND,
  LEX_OR_OR, LEX_OR,
  LEX_XOR,
  
  // Literals.
  LEX_IDENTIFIER, LEX_STRING, LEX_NUMBER,

  // Keywords.
  LEX_IF, LEX_ELSE, LEX_FOR, LEX_WHILE,
  LEX_ZILCH, LEX_RETURN, LEX_VOID, LEX_VAR,
  LEX_I8, LEX_I16, LEX_I32, LEX_I64,
  LEX_F32, LEX_F64,
  LEX_STR,
  // vectors are basically interpreted in the parser in a line (LEX_VECTOR, LEX_LESS, (?) , LEX_GREATER)
  LEX_VECTOR,

  LEX_ERROR, LEX_EOF
} LexType;

typedef struct {
    LexType type;
    const char *start;
    int length;
    int line;
} Token;

void init_lexer(const char *source);
Token scan_token();
const char* lex_datatype_to_str(LexType type);
OPCode lextype_to_opcode(LexType type);

#endif