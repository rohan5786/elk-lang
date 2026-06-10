#ifndef LEX_H
#define LEX_H

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
  LEX_SEMICOLON, LEX_SLASH, LEX_STAR,

  // One or two character tokens.
  LEX_BANG, LEX_BANG_EQUAL,
  LEX_EQUAL, LEX_EQUAL_EQUAL,
  LEX_GREATER, LEX_GREATER_EQUAL,
  LEX_LESS, LEX_LESS_EQUAL,
  
  // Literals.
  LEX_IDENTIFIER, LEX_STRING, LEX_NUMBER,

  // Keywords.
  LEX_AND, LEX_CLASS, LEX_ELSE, LEX_FALSE,
  LEX_FOR, LEX_FUN, LEX_IF, LEX_NIL, LEX_OR,
  LEX_PRINT, LEX_RETURN, LEX_SUPER, LEX_THIS,
  LEX_TRUE, LEX_VAR, LEX_WHILE,

  LEX_ERROR, LEX_EOF
} LexType;

typedef struct {
    LexType type;
    const char *start;
    int length;
    int line;
} Token;

void init_lexer(const char *source);
static bool end();

#endif