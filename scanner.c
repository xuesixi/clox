//
// Created by Yue Xue  on 11/17/24.
//

#include "scanner.h"
#include "common.h"
#include "string.h"

typedef struct {
    const char *start;
    const char *current;
    int line;
} Scanner;

Scanner scanner;

static bool is_end();

static char advance();

static char peek();

static char peek_next();

static bool match(char expected);

static Token make_token(TokenType type);

static void skip_whitespace();

static Token identifier_token();

static TokenType identifier_type();

static TokenType check_keyword(int start, int length, const char *rest, TokenType type);

static Token string_token();

static Token number_token();

static bool is_digit(char ch);

static bool is_alpha(char ch);

static Token error_token(const char *message);

//-----------------------------------------------------------

static TokenType check_keyword(int start, int length, const char *rest, TokenType type){
    if (scanner.current - scanner.start != start + length) {
        return TOKEN_IDENTIFIER;
    }
    if (memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    } else {
        return TOKEN_IDENTIFIER;
    }
}

static TokenType identifier_type() {
    switch (scanner.start[0]) {
        case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'c':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'l': return check_keyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': return check_keyword(2, 3, "nst", TOKEN_CONST);
                }
            }
            break;
        case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
        case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
        case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
            break;
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier_token() {
    while (is_alpha(peek()) || is_digit(peek())) {
        advance();
    }
    return make_token(identifier_type());
}

static inline bool is_alpha(char ch) {
    return (ch >= 'a' && ch <= 'z')|| (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

static inline bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

static Token number_token() {
    while (is_digit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && is_digit(peek_next())) {
        // Consume the ".".
        advance();

        while (is_digit(peek())) advance();
        return make_token(TOKEN_FLOAT);
    }
    return make_token(TOKEN_INT);
}

static Token string_token() {
    while (true) {
        char curr = peek();
        if (is_end()) {
            return error_token("The string is not terminated!");
        } else if (curr == '"') {
            advance();
            return make_token(TOKEN_STRING);
        } else if (curr == '\n') {
            scanner.line++;
        }
        advance();
    }
}

static void skip_whitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                continue;
            case '\n':
                scanner.line++;
                advance();
                continue;
            case '/':
                if (peek_next() == '/') {
                    while (peek() != '\n' && !is_end()) {
                        advance();
                    }
                    continue;
                } else {
                    return;
                }
            default:
                return;
        }
    }
}

static inline char peek() {
    return *scanner.current;
}

static char peek_next() {
    return scanner.current[1];
}

static bool match(char expected) {
    if (is_end()) {
        return false;
    }
    if (*scanner.current != expected) {
        return false;
    }
    scanner.current++;
    return true;
}

static inline char advance() {
    return *(scanner.current++);
}

static Token make_token(TokenType type) {
    Token token;
    token.type = type;
    token.line = scanner.line;
    token.start = scanner.start;
    token.length = (int) (scanner.current - scanner.start);
    return token;
}

static Token error_token(const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int) strlen(message);
    token.line = scanner.line;
    return token;
}

static inline bool is_end() {
    return *scanner.current == '\0';
}

//-----------------------------------------------------------

void init_scanner(const char *src) {
    scanner.start = src;
    scanner.current = src;
    scanner.line = 1;
}

Token scan_token() {
    skip_whitespace();
    scanner.start = scanner.current;
    if (is_end()) {
        return make_token(TOKEN_EOF);
    }

    char c = advance();
    if (is_digit(c)) {
        return number_token();
    }
    if (is_alpha(c)) {
        return identifier_token();
    }
    switch (c) {
        case '(':
            return make_token(TOKEN_LEFT_PAREN);
        case ')':
            return make_token(TOKEN_RIGHT_PAREN);
        case '{':
            return make_token(TOKEN_LEFT_BRACE);
        case '}':
            return make_token(TOKEN_RIGHT_BRACE);
        case ';':
            return make_token(TOKEN_SEMICOLON);
        case ',':
            return make_token(TOKEN_COMMA);
        case '.':
            return make_token(TOKEN_DOT);
        case '-':
            return make_token(TOKEN_MINUS);
        case '+':
            return make_token(TOKEN_PLUS);
        case '/':
            return make_token(TOKEN_SLASH);
        case '*':
            return make_token(TOKEN_STAR);
        case '%':
            return make_token(TOKEN_PERCENT);
        case '!':
            return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '>':
            return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '<':
            return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '=':
            return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '"':
            return string_token();
        default:
            return error_token("Unrecognized character!");
    }
}

