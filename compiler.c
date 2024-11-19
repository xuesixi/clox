//
// Created by Yue Xue  on 11/17/24.
//

#include "compiler.h"
#include "scanner.h"
#include "stdio.h"
#include "debug.h"
#include "stdlib.h"

typedef struct {
    Token previous;
    Token current;
    bool has_error;
    bool panic_mode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParserFunction)();

/**
 * prefix: 当作为第一个 token 的时候的解析函数
 * infix: 当作为第二个 token 的时候的解析函数
 * precedence：infix 的优先级
 */
typedef struct {
    ParserFunction prefix;
    ParserFunction infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk *compiling_chunk;

static void error_at(Token *token, const char *message);

static void error_at_current(const char *message);

static void error_at_previous(const char *message);

static void advance();

static void consume(TokenType type, const char *message);

static Chunk *current_chunk();

static void emit_byte(uint8_t byte);

static void emit_two_bytes(uint8_t byte1, uint8_t byte2);

static void end_compiler();

static void parse_precedence(Precedence precedence);

static void expression();

static void number();

static void unary();

static void grouping();

static void binary();

static int make_constant(Value value);

//-------------------------------------------------------------------------

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
        [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
        [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
        [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
        [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
        [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
        [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
        [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
        [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
        [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
        [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
        [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
        [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
        [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
        [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
        [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
        [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
        [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};


/**
 * 向后贪婪地解析所有连续的、优先级大于等于 precedence 的表达式
 * 以 1 + 2 * 3 * 4 - 5 为例，如果precedence 是+/-，那么会一直解析直到 curr 为-
 * @param precedence 指定的优先级
 */
static void parse_precedence(Precedence precedence) {
    advance();
    ParseRule *rule = &rules[parser.previous.type];
    if (rule->prefix == NULL) {
        error_at_previous("cannot be used as a value");
        return;
    }
    rule->prefix();
    while (precedence <= rules[parser.current.type].precedence) {
        // 之所以要使用 while 循环，是因为 infix 一般都不贪婪，但 parse_precedence 是贪婪的
        advance(); // 先 advance，因为 infix 函数一般假设操作符是 prev 而非 curr
        rules[parser.previous.type].infix();
    }
}

/**
 * 在左侧操作项已经被解析, previous 为操作符时，向后解析并完成这个 binary 表达式。
 * 这个解析本身并不贪婪。
 *
 */
static void binary() {
    TokenType type = parser.previous.type;
    ParseRule *rule = &rules[type];
    parse_precedence(rule->precedence + 1); // parse the right operand
    switch (type) {
        case TOKEN_PLUS:
            emit_byte(OP_ADD);
            break;
        case TOKEN_MINUS:
            emit_byte(OP_SUBTRACT);
            break;
        case TOKEN_STAR:
            emit_byte(OP_MULTIPLY);
            break;
        case TOKEN_SLASH:
            emit_byte(OP_DIVIDE);
            break;
        default:
            break;
    }
}

static void expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

static void unary() {
    TokenType operator = parser.previous.type;
    parse_precedence(PREC_UNARY);
    switch (operator) {
        case TOKEN_MINUS:
            emit_byte(OP_NEGATE);
            break;
        default:
            return;
    }
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    int index = make_constant(value);
    emit_two_bytes(OP_CONSTANT, index);
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "missing expected )");
}

static int make_constant(Value value) {
    int index = add_constant(current_chunk(), value);
    if (index > UINT8_MAX) {
        error_at_previous("Too many constants for a chunk");
        return 0;
    } else {
        return index;
    }
}

static void end_compiler() {
    emit_byte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
    if (!parser.has_error) {
        disassemble_chunk(current_chunk(), "disassembling the compiling result");
    }
#endif
}

static inline void emit_two_bytes(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

static Chunk *current_chunk() {
    return compiling_chunk;
}

static inline void emit_byte(uint8_t byte) {
    write_chunk(current_chunk(), byte, parser.previous.line);
}

static void consume(TokenType type, const char *message) {
    if (parser.current.type != type) {
        error_at_current(message);
    } else {
        advance();
    }
}

/**
 * 移动到下一个非错误的 token
 */
static void advance() {
    parser.previous = parser.current;
    while (true) {
        parser.current = scan_token();
        if (parser.current.type != TOKEN_ERROR) {
            break;
        } else {
            error_at_current(parser.current.start);
        }
    }
}

static inline void error_at_current(const char *message) {
    error_at(&parser.current, message);
}

static inline void error_at_previous(const char *message) {
    error_at(&parser.previous, message);
}

static void error_at(Token *token, const char *message) {
    if (parser.panic_mode) {
        return;
    }
    parser.panic_mode = true;
    fprintf(stderr, "[line %d] Compile Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at %.*s", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.has_error = true;
}

void show_tokens(const char *src) {
    // each line is in the format:
    // line number; token type number; lexeme
    init_scanner(src);
    int line = -1;
    while (1) {
        Token token = scan_token();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }
        printf("%2d  '%.*s'\n", token.type, token.length, token.start);
        if (token.type == TOKEN_EOF) {
            break;
        }
    }
}

bool compile(const char *src, Chunk *chunk) {
    init_scanner(src);
    compiling_chunk = chunk;
    advance();
    expression();
    parser.panic_mode = false;
    parser.has_error = false;
    return !parser.has_error;
}
