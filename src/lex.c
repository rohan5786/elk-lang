#include "lex.h"
#include "string.h"

#include <stdlib.h>
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

static void whitespace()
{
    while (1)
    {
        const char c = peek();
        // go all the way down for any 'whitespace'
        switch (c)
        {
            case ' ':
            case '\r':
            case '\v':
            case '\f':
            case '\t':
                next_char();
                break;
            case '\n': 
                lx.line++;
                next_char();
                break;
            case '/':
                if (peek_next() == '/')
                {
                    while (peek() != '\n' && !end()) 
                        next_char(); // single line cmnt
                }
                else return;
            default: 
                return; // if the nonwhite is reached
        } 
    }
}

static Token string()
{
    while (peek() != '"' && !end())
    {
        if (peek() == '\n') 
            lx.line++; // in this case we js wanna go past
        next_char();
    }
    next_char(); // passes the next char
    return make_token(LEX_STRING);
}

static bool digit(char c)
{
    return (c >= '0' && c <= '9');
}

static bool alpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static Token number()
{
    while (digit(peek()))
        next_char();
    // decimal caes
    if (peek() == '.' && digit(peek_next()))
    {
        next_char(); // take in the '.'
        while (digit(peek()))
            next_char();
    }
    return make_token(LEX_NUMBER);
}

static LexType choose_identifier(char *name)
{
    if (strcmp(name, "if") == 0) return LEX_IF; 
    if (strcmp(name, "else") == 0) return LEX_ELSE;
    if (strcmp(name, "for") == 0) return LEX_FOR; 
    if (strcmp(name, "while") == 0) return LEX_WHILE;

    if (strcmp(name, "null") == 0) return LEX_NULL; 
    if (strcmp(name, "return") == 0) return LEX_RETURN;
    if (strcmp(name, "void") == 0) return LEX_VOID;

    if (strcmp(name, "i8") == 0) return LEX_I8; 
    if (strcmp(name, "i16") == 0) return LEX_I16;
    if (strcmp(name, "i32") == 0) return LEX_I32;
    if (strcmp(name, "i64") == 0) return LEX_I64;

    if (strcmp(name, "f32") == 0) return LEX_F32;
    if (strcmp(name, "f64") == 0) return LEX_F64;

    if (strcmp(name, "var") == 0) return LEX_VAR;
    if (strcmp(name, "vector") == 0) return LEX_VECTOR;

    return LEX_IDENTIFIER;
}

static Token identifier()
{
    while (alpha(peek()) || digit(peek())) 
        next_char();

    // should use ptrdiff_t but it's too small anyways
    int len = (int) (lx.cur - lx.start); // exactly enough
    char* str = (char* )malloc(len + 1);
    if (str == NULL)
        return error_token("Not enough memory to scan identifier.\n");     

    memcpy(str, lx.start, len); // copies chars into str
    str[len] = '\0';

    Token tk = make_token(choose_identifier(str));

    free(str);

    return tk;
}

Token scan_token()
{
    whitespace(); // before anything
    lx.start = lx.cur;

    // checking! fun!; singles then dubles
    const char c = next_char();
    if (alpha(c)) return identifier(); // keywords and numbers
    if (digit(c)) return number();

    switch (c) {
        case '\0': return make_token(LEX_EOF);
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
        case '"': return string();
        case '&': return make_token(next_same('&') ? LEX_AND_AND : LEX_AND);
        case '|': return make_token(next_same('|') ? LEX_OR_OR : LEX_OR);
        case '^': return make_token(LEX_XOR);
        case '%': return make_token(LEX_PERCENT);
    }
    
    char err_msg[] = "unrecognizable token at '_'\0"; 
    err_msg[25] = c;
    return error_token(err_msg);
}

OPCode lextype_to_opcode(LexType type) {
    switch (type) {
        case LEX_PLUS: 
            return OP_ADD;
        case LEX_MINUS: 
            return OP_SUB;
        case LEX_STAR: 
            return OP_MULT;
        case LEX_SLASH: 
            return OP_DIV;
        case LEX_LESS_EQUAL: 
            return OP_LESS_EQL;
        case LEX_LESS: 
            return OP_LESS;
        case LEX_BANG_EQUAL: 
            return OP_NOT_EQL;
        case LEX_EQUAL_EQUAL: 
            return OP_EQUAL;
        case LEX_GREATER_EQUAL: 
            return OP_GREATER_EQL;
        case LEX_GREATER: 
            return OP_GREATER;
        case LEX_AND_AND: 
            return OP_AND;
        case LEX_OR_OR: 
            return OP_OR;
        case LEX_XOR: 
            return OP_XOR;
    }
}