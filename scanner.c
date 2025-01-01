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


static bool is_keyword(int start, int length, const char *rest) {
    if (scanner.current - scanner.start != start + length) {
        return false;
    }
    if (memcmp(scanner.start + start, rest, length) == 0) {
        return true;
    } else {
        return false;
    }
}

static TokenType check_keyword(int start, int length, const char *rest, TokenType type) {
    if (is_keyword(start, length, rest)) {
        return type;
    } else {
        return TOKEN_IDENTIFIER;
    }
}


static TokenType identifier_type() {
    switch (scanner.start[0]) {
        case 'a': {
            if (is_keyword(1, 1, "s")) {
                return TOKEN_AS;
            } else if (is_keyword(1, 2, "nd")) {
                return TOKEN_AND;
            } else {
                return TOKEN_IDENTIFIER;
            }
        }
        case 'b':
            return check_keyword(1, 4, "reak", TOKEN_BREAK);
        case 'c':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'l':
                        return check_keyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': {
                        if (is_keyword(2, 3, "nst")) {
                            return TOKEN_CONST;
                        } else if (is_keyword(2, 6, "ntinue")) {
                            return TOKEN_CONTINUE;
                        } else {
                            return TOKEN_IDENTIFIER;
                        }
                    }
                    case 'a': {
                        if (is_keyword(2, 3, "tch")) {
                            return TOKEN_CATCH;
                        } else if (is_keyword(2, 2, "se")) {
                            return TOKEN_CASE;
                        } else {
                            return TOKEN_IDENTIFIER;
                        }
                    }
                }
            }
            break;
        case 'e': {
            if (is_keyword(1, 3, "lse")) {
                return TOKEN_ELSE;
            } else if (is_keyword(1, 5, "xport")) {
                return TOKEN_EXPORT;
            } else {
                return TOKEN_IDENTIFIER;
            }
        }
        case 'd':
            return check_keyword(1, 6, "efault", TOKEN_DEFAULT);
        case 'i': {
            if (is_keyword(1, 1, "f")) {
                return TOKEN_IF;
            } else if (is_keyword(1, 5, "mport")) {
                return TOKEN_IMPORT;
            } else if (is_keyword(1, 1, "n")) {
                return TOKEN_IN;
            } else {
                return TOKEN_IDENTIFIER;
            }
        }
        case 'n':
            return check_keyword(1, 2, "il", TOKEN_NIL);
        case 'o':
            return check_keyword(1, 1, "r", TOKEN_OR);
        case 'p':
            return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r':
            return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 's':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'u':
                        return check_keyword(2, 3, "per", TOKEN_SUPER);
                    case 'w':
                        return check_keyword(2, 4, "itch", TOKEN_SWITCH);
                    case 't':
                        return check_keyword(2, 4, "atic", TOKEN_STATIC);
                }
            }
            break;
        case 'v':
            return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w':
            return check_keyword(1, 4, "hile", TOKEN_WHILE);
        case 'f':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'a':
                        return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o':
                        return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u':
                        return check_keyword(2, 1, "n", TOKEN_FUN);
                }
            }
            break;
        case 't':
            if (scanner.current - scanner.start > 1) {
                switch (scanner.start[1]) {
                    case 'h': {
                        if (is_keyword(2, 2, "is")) {
                            return TOKEN_THIS;
                        } else if (is_keyword(2, 3, "row")) {
                            return TOKEN_THROW;
                        } else {
                            return TOKEN_IDENTIFIER;
                        }
                    }
                    case 'r': {
                        if (is_keyword(2, 1, "y")) {
                            return TOKEN_TRY;
                        } else if (is_keyword(2, 2, "ue")) {
                            return TOKEN_TRUE;
                        } else {
                            return TOKEN_IDENTIFIER;
                        }
                    }
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
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
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

static Token label_token() {
    while (true) {
        while (peek() != '\n' && !is_end()) {
            advance();
        }
        if (peek() == '\n') {
            return make_token(TOKEN_LABEL);
        }
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
                } else if (peek_next() == '*') {
                    advance();
                    advance();
                    while ((peek() != '*' || peek_next() != '/') && !is_end()) {
                        if (peek() == '\n') {
                            scanner.line++;
                        }
                        advance();
                    }
                    if (is_end()) {
                        printf("unterminated comment\n");
                    } else {
                        advance();
                        advance();
                        continue;
                    }
                } else {
                    return;
                }
            case '#':
                while (peek() != '\n' && !is_end()) {
                    advance();
                }
                continue;
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
        case '|':
            return make_token(TOKEN_PIPE);
        case '@':
            return make_token(TOKEN_AT);
        case ')':
            return make_token(TOKEN_RIGHT_PAREN);
        case '{':
            return make_token(TOKEN_LEFT_BRACE);
        case '}':
            return make_token(TOKEN_RIGHT_BRACE);
        case '[':
            return make_token(TOKEN_LEFT_BRACKET);
        case ']':
            return make_token(TOKEN_RIGHT_BRACKET);
        case ';':
            return make_token(TOKEN_SEMICOLON);
        case ':':
            return make_token(TOKEN_COLON);
        case ',':
            return make_token(TOKEN_COMMA);
        case '.': {
            if (match('.')) {
                if (match('.')) {
                    return make_token(TOKEN_DOT_DOT_DOT);
                } else {
                    return error_token("Unrecognized character!");
                }
            } else {
                return make_token(TOKEN_DOT);
            }
        }
        case '-':
            return make_token(match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
        case '+':
            return make_token(match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
        case '/':
            return make_token(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
        case '*':
            if (match('=')) return make_token(TOKEN_STAR_EQUAL);
            if (match('*')) return make_token(TOKEN_STAR_STAR);
            return make_token(TOKEN_STAR);
        case '%':
            return make_token(match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
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
        case '#':
            return label_token();
        case '$':
            return make_token(TOKEN_DOLLAR);
        default:
            return error_token("Unrecognized character!");
    }
}

