#include "lex.h"
#include "string.h"

#include <stdbool.h>

Lexer lx;

void init_lexer(const char *source)
{
    lx.start = source;
    lx.cur = source;
    lx.line = 1;
}

static bool end()
{
    return *lx.cur == '\0';
}

static Token make_token(LexType type)
{
    Token tk;
    tk.type = type;
    // address math
    tk.start = lx.start;
    tk.length = (int) (lx.cur - lx.start); 
    tk.line = lx.line;
    return tk;
}

static Token error_token(const char *msg)
{
    Token tk;
    tk.type = LEX_ERROR;
    tk.start = msg;
    tk.length = (int) strlen(msg);
    tk.line = lx.line;
    return tk;
}

// returns current, advances to next
static char next_char()
{
    lx.cur++; // increment mem addy
    return *(lx.cur - 1);
}

static bool next_same(char wanted)
{
    // lx.cur is at the next one after next_char() is run
    if (end() || *lx.cur != wanted) 
        return false;
    lx.cur++;
    return true;
}

static char peek()
{
    return *lx.cur;
}

static char peek_next()
{
    if (end()) 
        return '\0';
    return *(lx.cur + 1);
}

static void skip_whitespace()
{
    while (1)
    {
        const char c = peek();
        // go all the way down for any 'whitespace'
        switch (c)
        {
            case ' ':
            case '/':
                if (peek_next() == '/')
                {
                    while (peek() != '\n' && !end()) 
                        next_char(); // single line cmnt
                }
                else return;
            case '\r':
            case '\v':
            case '\f':
            case '\n': 
                lx.line++;
            case '\t':
                next_char();
                break;
            default: return; // if the nonwhite is reached
        } 
    }
}

// TODO: Keywords, Literals, Multi-line comments
Token scan_token()
{
    skip_whitespace(); // before anything
    lx.start = lx.cur;

    // checking! fun!; singles then dubles
    const char c = next_char();
    switch (c) {
        case '(': return make_token(LEX_LEFT_PAREN);
        case ')': return make_token(LEX_RIGHT_PAREN);
        case '{': return make_token(LEX_LEFT_BRACE);
        case '}': return make_token(LEX_RIGHT_BRACE);
        case ';': return make_token(LEX_SEMICOLON);
        case ',': return make_token(LEX_COMMA);
        case '.': return make_token(LEX_DOT);
        case '-': return make_token(LEX_MINUS);
        case '+': return make_token(LEX_PLUS);
        case '/': return make_token(LEX_SLASH);
        case '*': return make_token(LEX_STAR);
        case '!': return make_token(next_same('=') ? LEX_BANG_EQUAL : LEX_BANG);
        case '=': return make_token(next_same('=') ? LEX_EQUAL_EQUAL : LEX_EQUAL);
        case '<': return make_token(next_same('=') ? LEX_LESS_EQUAL : LEX_LESS);
        case '>': return make_token(next_same('=') ? LEX_GREATER_EQUAL : LEX_GREATER);
    }


    if (end()) 
        return make_token(LEX_EOF);
    
    return error_token("Idk bru");
}