//
// Created by Yue Xue  on 11/17/24.
//

#ifndef CLOX_SCANNER_H
#define CLOX_SCANNER_H

typedef enum TokenType{
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACKET,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE, TOKEN_RIGHT_BRACKET,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS, TOKEN_COLON,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR, TOKEN_PERCENT,
    TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL, TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_PERCENT_EQUAL,
    TOKEN_STAR_STAR,

    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,

    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_FLOAT, TOKEN_INT, TOKEN_LABEL,

    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE, TOKEN_CONST,
    TOKEN_SWITCH, TOKEN_CASE, TOKEN_DEFAULT, TOKEN_CONTINUE, TOKEN_BREAK,
    TOKEN_STATIC,

    TOKEN_ERROR, TOKEN_EOF
} TokenType;

typedef struct Token{
    TokenType type;
    const char *start;
    int length;
    int line;
} Token;


void init_scanner(const char* source);
Token scan_token();

#endif //CLOX_SCANNER_H
