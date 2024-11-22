//
// Created by Yue Xue  on 11/17/24.
//

/**
 * compiler 中并不存在 token 的列表或者数组。
 * 我们永远只有只在栈中保留两个 token：prev 和 current。
 * 每当我们要解析下一个的时候，才用 advance 函数（它调用了 scan_token 函数）去读取下一个 token
 * 
 * */

#include "compiler.h"
#include "object.h"
#include "scanner.h"
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

static void float_num();

static void int_num();

static void literal();

static void unary();

static void grouping();

static void binary();

static void string();

static int make_constant(Value value);

//-------------------------------------------------------------------------

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {grouping, NULL, PREC_NONE},
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
        [TOKEN_PERCENT]       = {NULL,     binary, PREC_FACTOR},
        [TOKEN_BANG]          = {unary,     NULL,   PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL,     binary,   PREC_EQUALITY},
        [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL,     binary,   PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL,     binary,   PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL,     binary,   PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL,     binary,   PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL,     binary,   PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
        [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
        [TOKEN_FLOAT]         = {float_num, NULL, PREC_NONE},
        [TOKEN_INT]           = {int_num, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
        [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
        [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
        [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
        [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static void string() {
    String *str = string_copy(parser.previous.start + 1, parser.previous.length - 2);
    Value value = ref_value((Object*) str);
    int index = make_constant(value);
    emit_two_bytes(OP_CONSTANT, index);
}

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
        case TOKEN_PERCENT:
            emit_byte(OP_MOD);
            break;
        case TOKEN_LESS:
            emit_byte(OP_LESS);
            break;
        case TOKEN_GREATER:
            emit_byte(OP_GREATER);
            break;
        case TOKEN_EQUAL_EQUAL:
            emit_byte(OP_EQUAL);
            break;
        case TOKEN_LESS_EQUAL:
            emit_byte(OP_GREATER);
            emit_byte(OP_NOT);
            break;
        case TOKEN_GREATER_EQUAL:
            emit_byte(OP_LESS);
            emit_byte(OP_NOT);
            break;
        case TOKEN_BANG_EQUAL:
            emit_byte(OP_EQUAL);
            emit_byte(OP_NOT);
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
        case TOKEN_BANG:
            emit_byte(OP_NOT);
            break;
        default:
            return;
    }
}

static void float_num() {
    double value = strtod(parser.previous.start, NULL);
    int index = make_constant(float_value(value));
    emit_two_bytes(OP_CONSTANT, index);
}

static void int_num() {
    int value = (int) strtol(parser.previous.start, NULL, 10);
    int index = make_constant(int_value(value));
    emit_two_bytes(OP_CONSTANT, index);
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_NIL:
            emit_byte(OP_NIL);
            break;
        case TOKEN_TRUE:
            emit_byte(OP_TRUE);
            break;
        case TOKEN_FALSE:
            emit_byte(OP_FALSE);
            break;
        default:
            error_at_previous("No such literal");
            return;
    }
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "missing expected )");
}

/**
 * 将指定的 value 作为常数储存到 vm.code.constants 之中，然后返回其索引
 * 该函数包装了 add_constant
 * */
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

/**
 * 将目标源代码编译成字节码，写入到目标 chunk 中
 * */
bool compile(const char *src, Chunk *chunk) {
    init_scanner(src);
    compiling_chunk = chunk;
    advance();
    expression();
    end_compiler();
    bool has_error = parser.has_error;
    parser.panic_mode = false;
    parser.has_error = false;
    return !has_error;
}
