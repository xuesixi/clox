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
#include "table.h"
#include "string.h"
#include "stdlib.h"
#include "debug.h"

typedef struct Parser{
    Token previous;
    Token current;
    Table lexeme_table;
    int break_point;
    int old_break_point;
    int continue_point;
    int continue_point_depth;
    int old_continue_point;
    int old_continue_point_depth;
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

typedef void (*ParserFunction)(bool can_assign);

/**
 * prefix: 当作为第一个 token 的时候的解析函数.
 * infix: 当作为第二个 token 的时候的解析函数。在调用该函数时，该token处于prev的位置。
 * precedence：infix 的优先级
 */
typedef struct ParseRule{
    ParserFunction prefix;
    ParserFunction infix;
    Precedence precedence;
} ParseRule;

typedef struct Local {
    Token name;
    int depth;
    bool is_const;
} Local;

typedef struct Scope {
    Local locals[UINT8_MAX + 1];
    int count;
    int depth;
} Scope;

Parser parser;
Chunk *compiling_chunk;
Scope *current_scope;


static inline bool lexeme_equal(Token *a, Token *b);

static inline void begin_scope();

static void declare_local(bool is_const);

static int resolve_local(Scope *scope, Token *token, bool *is_const);

static inline void end_scope();

static void clear_scope(int to);

static inline void block_statement();

static void init_scope(Scope *scope);

static void error_at(Token *token, const char *message);

static void error_at_current(const char *message);

static void error_at_previous(const char *message);

static void advance();

static void consume(TokenType type, const char *message);

static Chunk *current_chunk();

static void emit_byte(uint8_t byte);

static void emit_two_bytes(uint8_t byte1, uint8_t byte2);

static void emit_uint16(uint16_t value);

static void emit_constant(int index);

static void end_compiler();

static void parse_precedence(Precedence precedence);

static void expression();

static void float_num(bool can_assign);

static void int_num(bool can_assign);

static void literal(bool can_assign);

static void variable(bool can_assign);

static void unary(bool can_assign);

static void grouping(bool can_assign);

static void binary(bool can_assign);

static void string(bool can_assign);

static void parse_continue(bool can_assign);

static void parse_break(bool can_assign);

static void and(bool can_assign);

static void or(bool can_assign);

static void declaration();

static void statement();

static void if_statement();

static void loop_back(int start);

static void while_statement();

static void for_statement();

static void switch_statement();

static void patch_jump(int from);

static int emit_jump(uint8_t jump_op);

static void emit_goto(int dest);

static void var_declaration();

static void const_declaration();

static int parse_var_declaration(bool is_const);

static int identifier_constant(Token *name);

static void named_variable(Token *name, bool can_assign);

static void print_statement();

static void expression_statement();

static bool match(TokenType type);

static bool check(TokenType type);

static void synchronize();

static int make_constant(Value value);

//-------------------------------------------------------------------------

ParseRule rules[] = {
        [TOKEN_LEFT_PAREN]    = {grouping, NULL, PREC_NONE},
        [TOKEN_RIGHT_PAREN]   = {NULL, NULL, PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL, NULL, PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL, NULL, PREC_NONE},
        [TOKEN_COMMA]         = {NULL, NULL, PREC_NONE},
        [TOKEN_DOT]           = {NULL, NULL, PREC_NONE},
        [TOKEN_MINUS]         = {unary, binary, PREC_TERM},
        [TOKEN_PLUS]          = {NULL, binary, PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL, NULL, PREC_NONE},
        [TOKEN_SLASH]         = {NULL, binary, PREC_FACTOR},
        [TOKEN_STAR]          = {NULL, binary, PREC_FACTOR},
        [TOKEN_PERCENT]       = {NULL, binary, PREC_FACTOR},
        [TOKEN_BANG]          = {unary, NULL, PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL, binary, PREC_EQUALITY},
        [TOKEN_EQUAL]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL, binary, PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL, binary, PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL, binary, PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL, binary, PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {variable, NULL, PREC_NONE},
        [TOKEN_STRING]        = {string, NULL, PREC_NONE},
        [TOKEN_FLOAT]         = {float_num, NULL, PREC_NONE},
        [TOKEN_INT]           = {int_num, NULL, PREC_NONE},
        [TOKEN_AND]           = {NULL, and, PREC_AND},
        [TOKEN_CLASS]         = {NULL, NULL, PREC_NONE},
        [TOKEN_ELSE]          = {NULL, NULL, PREC_NONE},
        [TOKEN_FALSE]         = {literal, NULL, PREC_NONE},
        [TOKEN_FOR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_FUN]           = {NULL, NULL, PREC_NONE},
        [TOKEN_IF]            = {NULL, NULL, PREC_NONE},
        [TOKEN_NIL]           = {literal, NULL, PREC_NONE},
        [TOKEN_OR]            = {NULL, or, PREC_OR},
        [TOKEN_PRINT]         = {NULL, NULL, PREC_NONE},
        [TOKEN_RETURN]        = {NULL, NULL, PREC_NONE},
        [TOKEN_SUPER]         = {NULL, NULL, PREC_NONE},
        [TOKEN_THIS]          = {NULL, NULL, PREC_NONE},
        [TOKEN_TRUE]          = {literal, NULL, PREC_NONE},
        [TOKEN_VAR]           = {NULL, NULL, PREC_NONE},
        [TOKEN_WHILE]         = {NULL, NULL, PREC_NONE},
        [TOKEN_CONTINUE]      = {parse_continue, NULL, PREC_NONE},
        [TOKEN_BREAK]         = {parse_break, NULL, PREC_NONE},
        [TOKEN_ERROR]         = {NULL, NULL, PREC_NONE},
        [TOKEN_EOF]           = {NULL, NULL, PREC_NONE},
};

static inline bool lexeme_equal(Token *a, Token *b) {
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static inline void save_continue_point() {
    parser.old_continue_point = parser.continue_point;
    parser.old_continue_point_depth = parser.continue_point_depth;
    parser.continue_point = current_chunk()->count;
    parser.continue_point_depth = current_scope->depth;
}

static inline void restore_continue_point() {
    parser.continue_point = parser.old_continue_point;
    parser.continue_point_depth = parser.old_continue_point_depth;
}

static inline void save_break_point() {
    parser.old_break_point = parser.break_point;
    parser.break_point = current_chunk()->count;
    // printf("the breka point is %d\n", parser.break_point);
}

static inline void restore_break_point() {
    parser.break_point = parser.old_break_point;
}


static void string(bool can_assign) {
    (void )can_assign;
    String *str = string_copy(parser.previous.start + 1, parser.previous.length - 2);
    Value value = ref_value((Object *) str);
    int index = make_constant(value);
    emit_constant(index);
}

/**
 * 从curr开始，向后贪婪地解析所有连续的、优先级大于等于 precedence 的表达式
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
    bool can_assign = precedence <= PREC_ASSIGNMENT;
    rule->prefix(can_assign);
    while (precedence <= rules[parser.current.type].precedence) {
        // 之所以要使用 while 循环，是因为 infix 一般都不贪婪，但 parse_precedence 是贪婪的
        advance(); // 先 advance，因为 infix 函数一般假设操作符是 prev 而非 curr

        rules[parser.previous.type].infix(can_assign); // 这里的can_assign似乎意义不大。多数infix用不到它。对于assign对来，它自己的prefix就会处理。
    }
}

static void declaration() {
    if (match(TOKEN_VAR)) {
        var_declaration();
    } else if (match(TOKEN_CONST)) {
        const_declaration();
    } else {
        statement();
    }
    if (parser.panic_mode) {
        synchronize();
    }
}

static void var_declaration() {

    int index = parse_var_declaration(false);

    if (match(TOKEN_EQUAL)) {

        expression();
    } else {
        emit_byte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "A semicolon is needed to terminate the var statement");

    if (current_scope->depth == 0) {
        emit_two_bytes(OP_DEFINE_GLOBAL, index);
    } else {
        current_scope->locals[current_scope->count - 1].depth = current_scope->depth;
    }
    // 局部变量不需要额外操作。先前把初始值或者nil置入栈中就足够了。
}

static void const_declaration() {

    int index = parse_var_declaration(true);
    consume(TOKEN_EQUAL, "A const variable must be initialized");
    expression();
    consume(TOKEN_SEMICOLON, "A semicolon is needed to terminate the const statement");

    if (current_scope->depth == 0) {
        emit_two_bytes(OP_DEFINE_GLOBAL_CONST, index);
    } else {
        current_scope->locals[current_scope->count - 1].depth = current_scope->depth;
    }
    // 局部变量不需要额外操作。先前把初始值或者nil置入栈中就足够了。
}

/**
 * clox中，当一个语句（statement）执行完毕后，它必然会消耗掉期间产生的所有栈元素。
 * 本地变量的申明是唯一的可以产生“超单一语句”的栈元素的语句。它会在block结束后被全部清除。
 * 因为本地变量是唯一可以长时间存在的元素，一个本地变量在locals中的索引必然就是它在stack中的索引。
 */
static void declare_local(bool is_const) {
    if (current_scope->count >= STACK_MAX) {
        error_at_previous("too many local variables");
        return;
    }

    Token *token = &parser.previous;

    // 检查同一层scope中是否存在同名的变量
    for (int i = current_scope->count - 1; i >= 0; i--) {
        Local *curr = current_scope->locals + i;
        if (curr->depth < current_scope->depth) {
            break;
        } else if (lexeme_equal(&curr->name, token)) {
            error_at_previous("You cannot re-declare a local variable");
            return;
        }
    }

    // 添加新的local变量
    Local *local = current_scope->locals + current_scope->count;
    local->name = parser.previous;
    local->is_const = is_const;
    local->depth = -1; // 在完成初始化之后，再设置为正确值。这是为了防止初始化中用到自己，比如`var num = num + 10;`
    current_scope->count++;
}

/**
 * 给定一个token，判断它在local变量表中的位置。
 * 该函数从后往前检查，因此后申明的变量会覆盖前申明的同名变量。
 * 如果不存在，返回-1
 * @param token 要检查的变量名
 * @return 该变量在local变量表的位置（同时也是vm的栈中的索引）
 */
static int resolve_local(Scope *scope, Token *token, bool *is_const) {
    for (int i = scope->count - 1; i >= 0; i--) {
        Local *curr = scope->locals + i;
        if (lexeme_equal(token, &curr->name)) {
            if (curr->depth == -1) {
                error_at_previous("cannot use a variable in its own initialization");
                return -1;
            } else{
                if (is_const != NULL) {
                    *is_const = curr->is_const;
                }
                return i;
            }
        }
    }
    return -1;
}

/**
 * 解析一个标识符，如果是全局变量，则将标识符添加为常量。如果是局部变量，申明之
 * @return 如果是全局变量，返回索引。否则，返回-1
 */
static inline int parse_var_declaration(bool is_const) {
    consume(TOKEN_IDENTIFIER, "An identifier is expected here");

    if (current_scope->depth > 0) {
        // 如果是local
        declare_local(is_const);
        return -1;
    } else {
        // 否则是global
        return identifier_constant(&parser.previous);
    }
}

/**
 * 将标识符token添加入常数中，然后返回其索引
 * @return 常数中的索引
 */
static inline int identifier_constant(Token *name) {
    String *str = string_copy(name->start, name->length);
    return make_constant(ref_value((Object*)str));
}

static void statement() {
    if (match(TOKEN_PRINT)) {
        print_statement();
    }else if(match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block_statement();
        end_scope();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_FOR)){
        for_statement();
    }else if (match(TOKEN_SWITCH)){
        switch_statement();
    } else {
        expression_statement();
    }
}

/**
 * 生成一个jump指令。后面两个操作数是占位符。
 * 该函数需要配合patch_jump使用。
 * @param jump_op jump的类型
 * @return 最后一个操作数的索引 + 1
 */
static int emit_jump(uint8_t jump_op) {
    emit_byte(jump_op); // 0
    emit_byte(0xff);// 1
    emit_byte(0xff); // 2
    return current_chunk()->count; // 3
}

static void emit_goto(int dest) {
    int current_position = current_chunk()->count + 3;
    // printf("curr is %d, goto destination is %d\n", current_position, dest);
    int diff = dest - current_position;
    if (diff > 0 ) {
        emit_byte(OP_JUMP);
        emit_uint16(diff);
    } else if (diff < 0 ){
        emit_byte(OP_JUMP_BACK);
        emit_uint16(-diff);
    }
}

/**
 * 修改from处的jump指令的offset，使其指向本处。
 * 跳转后, PC指向该函数后面的下一个命令。把该函数当成跳转的标签即可。
 * 那个jump指令的op的索引是from-3
 * @param from
 */
static void patch_jump(int from) {
    uint16_t diff = current_chunk()->count - from;
    uint8_t i0, i1;
    u16_to_u8(diff, &i0, &i1);
    current_chunk()->code[from - 2] = i0;
    current_chunk()->code[from - 1] = i1;
}

/**
 * condition
 *
 * pop, if false, jump -> else
 *
 * then:
 *     ...
 *     jump to [after]
 *
 * else:
 *     ...
 *
 * after:
 */
static void if_statement() {
    consume(TOKEN_LEFT_PAREN, "A ( is expected after if");
    // condition
    expression(); 
    consume(TOKEN_RIGHT_PAREN, "A ) is expected after if");

    // jump to else if false
    int to_else = emit_jump(OP_JUMP_IF_FALSE_POP);

    statement();
    // jump to after
    int to_after = emit_jump(OP_JUMP);

    //else
    patch_jump(to_else);

    if (match(TOKEN_ELSE)) {
        statement();
    }

    // after
    patch_jump(to_after);
}

static void loop_back(int start) {
    emit_byte(OP_JUMP_BACK);
    int diff = current_chunk()->count - start + 2;
    uint8_t i0, i1;
    u16_to_u8(diff, &i0, &i1);
    emit_two_bytes(i0, i1);
}

/**
 * start:
 * 
 * continue point:
 * condition:
 *     expression
 * break point:
 *     pop, if false, jump -> end
 * 
 * body:
 *     pop condition
 *     statement
 *     jump -> condition
 * 
 * end:
 * */
static void while_statement() {
    consume(TOKEN_LEFT_PAREN, "A ( is expected after while");

    save_continue_point();
    int condition = current_chunk()->count;

    // condition:
    expression(); 
    consume(TOKEN_RIGHT_PAREN, "A ) is expected after while");
    save_break_point();
    int to_end = emit_jump(OP_JUMP_IF_FALSE_POP);

    // body: 
    statement();

    loop_back(condition);

    // end: 
    patch_jump(to_end);

    restore_continue_point();
    restore_break_point();
}

/**
 * jump -> start
 * 
 * temp:
 *     jump -> end
 * 
 * start:
 *     expression_0
 * 
 * case 1:
 *     expression_1
 *     if not equal, jump -> case 2
 *     pop expression_1
 *     pop expression_0
 *     statement...
 *     jump -> temp
 * 
 * case 2:
 *     pop expression_1
 *     expression_2
 *     if not equal, jump -> default
 *     pop expression_2
 *     pop expression_0
 *     statement...
 *     jump -> temp
 * 
 * default:
 *     pop expression_2
 *     pop exression_1
 *     statement....
 * 
 * end:
 * 
 * */
static void switch_statement() {
    consume(TOKEN_LEFT_PAREN, "A ( is expected after switch");

    int start = emit_jump(OP_JUMP);

    int temp = current_chunk()->count;
    int to_end = emit_jump(OP_JUMP);

    patch_jump(start);

    expression();

    consume(TOKEN_RIGHT_PAREN, "A ) is expected after switch");
    consume(TOKEN_LEFT_BRACE, "A { is expected after switch");
    int bridge = -1;
    while (!check(TOKEN_EOF) && match(TOKEN_CASE)) {
        if (bridge != -1) {
            patch_jump(bridge);
            emit_byte(OP_POP);
        }
        if (match(TOKEN_NIL) || match(TOKEN_FALSE) || match(TOKEN_TRUE)) {
            literal(false);
        } else if (match(TOKEN_FLOAT)) {
            float_num(false);
        } else if (match(TOKEN_INT)) {
            int_num(false);
        } else if (match(TOKEN_STRING)) {
            string(false);
        } else {
            error_at_current("only constant values can be used as switch cases");
            return;
        }
        consume(TOKEN_COLON, "A : is needed after each case");
        bridge = emit_jump(OP_JUMP_IF_NOT_EQUAL);
        emit_byte(OP_POP);
        emit_byte(OP_POP);

        while (!check(TOKEN_EOF) && !check(TOKEN_CASE) && !check(TOKEN_DEFAULT) && !check(TOKEN_RIGHT_BRACE)) {
            statement();
        }
        loop_back(temp);
    }

    patch_jump(bridge);
    emit_byte(OP_POP);

    // if the switch statement has no any cases, we only have one expression to pop
    // otherwise, we need to pop two
    if (bridge != -1) {
        emit_byte(OP_POP);
    }

    if (match(TOKEN_DEFAULT)) {
        consume(TOKEN_COLON, "A : is needed after each case");
        while (!check(TOKEN_EOF) && !check(TOKEN_RIGHT_BRACE)) {
            statement();
        }
    }

    patch_jump(to_end);
    consume(TOKEN_RIGHT_BRACE, "A } is expected after switch");
}

/*
 * for (initialize condition increment) statement
 * 
 * begin_scope
 * initialize
 * 
 * condition: 
 *     expression
 * break point:
 *     pop, if false, jump -> end
 *     jump -> body
 * 
 * continue_point:
 *
 * increment:
 *     exression
 *     pop increment expression
 *     jump -> condition
 * 
 * body:
 *     pop condition
 *     statement
 *     jump -> increment
 * 
 * end:
 *     pop condition
 * 
 * end_scope
 */
static void for_statement() {

    consume(TOKEN_LEFT_PAREN, "A ( is expected after for");
    begin_scope();

    // initialize
    if (match(TOKEN_SEMICOLON)) {

    } else if (match(TOKEN_VAR)) {
        var_declaration();
    } else {
        expression_statement();
    }

    int condition = current_chunk()->count;

    // condition
    if (!match(TOKEN_SEMICOLON)) {
        expression(); // not expression_statement() because we want to keep the condition code
        consume(TOKEN_SEMICOLON, "the for initializer needs a ;");
    } else {
        emit_byte(OP_TRUE);
    }

    save_break_point();
    int to_end = emit_jump(OP_JUMP_IF_FALSE_POP);
    int to_body = emit_jump(OP_JUMP);

    int increment = current_chunk()->count;

    save_continue_point();

    // increment
    if (!match(TOKEN_RIGHT_PAREN)) {
        expression(); 
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "A ) is expected after for");
    }

    loop_back(condition);

    // body
    patch_jump(to_body);
    statement();
    loop_back(increment);

    // end
    patch_jump(to_end);

    restore_continue_point();
    restore_break_point();

    end_scope();

}

static inline void begin_scope() {
    current_scope->depth ++;
}

/**
 * 清除所有比to层级更深的local变量。对于每一个被清除的local变量，产生一个POP指令
 * */
static void clear_scope(int to) {
    while (current_scope->count > 0) {
        // 当前顶层的那个local
        Local *curr = current_scope->locals + current_scope->count - 1;

        // 如果这个local的层级更深，那么我们需要将其移除
        if (curr->depth > to) {
            emit_byte(OP_POP);
            current_scope->count--;
        } else {
            break;
        }
    }
}

/**
 * 对于每一个比to层级更深的变量，产生一个POP指令。但该函数不会修改count
 * */
static void emit_pops_to_clear(int to) {
    Local *curr = current_scope->locals + current_scope->count - 1;
    while (curr >= current_scope->locals) {
        // 如果这个local的层级更深，那么我们需要将其移除
        if (curr->depth > to) {
            emit_byte(OP_POP);
            curr --;
        } else {
            break;
        }
    }
}

static void parse_break(bool can_assign) {
    (void) can_assign;
    if (parser.break_point < 0) {
        error_at_previous("cannot use break outside of a loop");
        return;
    }
    emit_pops_to_clear(parser.continue_point_depth);
    emit_byte(OP_FALSE);
    emit_goto(parser.break_point);
}

static void parse_continue(bool can_assign) {
    (void) can_assign;
    if (parser.continue_point < 0) {
        error_at_previous("cannot use continue outside of a loop");
        return;
    }
    emit_pops_to_clear(parser.continue_point_depth);
    emit_goto(parser.continue_point);
}


/**
 * 当离开scope的时候，把所有当前scope的local变量移除
 */
static void end_scope() {
    current_scope->depth --;

    clear_scope(current_scope->depth);
}

static void block_statement() {
    while (!check(TOKEN_EOF) && !check(TOKEN_RIGHT_BRACE)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "A block should terminate with a }");
}

static inline void print_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "A semicolon is needed to terminated the statement");
    emit_byte(OP_PRINT);
}

static inline void expression_statement() {
    expression();
    consume(TOKEN_SEMICOLON, "A semicolon is needed to terminated the statement");
    emit_byte(OP_POP);
}

static inline void expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

/**
 * 标识符的前缀解析函数
 */
static inline void variable(bool can_assign) {
    named_variable(&parser.previous, can_assign);
}

static void arithmetic_equal(OpCode set_op, OpCode get_op, int index) {
    switch (parser.previous.type) {
        case TOKEN_EQUAL: {
            expression();
            emit_two_bytes(set_op, index);
            break;
        }
        case TOKEN_PLUS_EQUAL: {
            expression();
            emit_two_bytes(get_op, index);
            emit_byte(OP_ADD);
            emit_two_bytes(set_op, index);
            break;
        }
        case TOKEN_MINUS_EQUAL: {
            expression();
            emit_two_bytes(get_op, index);
            emit_byte(OP_SUBTRACT);
            emit_two_bytes(set_op, index);
            break;
        }
        case TOKEN_STAR_EQUAL: {
            expression();
            emit_two_bytes(get_op, index);
            emit_byte(OP_MULTIPLY);
            emit_two_bytes(set_op, index);
            break;
        }
        case TOKEN_SLASH_EQUAL: {
            expression();
            emit_two_bytes(get_op, index);
            emit_byte(OP_DIVIDE);
            emit_two_bytes(set_op, index);
            break;
        }
        case TOKEN_PERCENT_EQUAL: {
            expression();
            emit_two_bytes(get_op, index);
            emit_byte(OP_MOD);
            emit_two_bytes(set_op, index);
            break;
        }
        default:
            IMPLEMENTATION_ERROR("this should not happen");
            return;
    }

}

static inline bool match_assign() {
    return match(TOKEN_EQUAL)
           || match(TOKEN_PLUS_EQUAL)
           || match(TOKEN_MINUS_EQUAL)
           || match(TOKEN_STAR_EQUAL)
           || match(TOKEN_SLASH_EQUAL)
           || match(TOKEN_PERCENT_EQUAL);
}

/**
 * 解析global/local变量的get/set
 * @param name
 * @param can_assign
 */
static void named_variable(Token *name, bool can_assign) {
    int set_op = OP_SET_LOCAL;
    int get_op = OP_GET_LOCAL;
    bool is_const = false;
    int index = resolve_local(current_scope, name, &is_const);
    if (index == -1) {
        index = identifier_constant(name);
        set_op = OP_SET_GLOBAL;
        get_op = OP_GET_GLOBAL;
    }
    if (can_assign && match_assign()) {
        if (is_const) {
            error_at_previous("cannot re-assign a const variable");
            return;
        }
        arithmetic_equal(set_op, get_op, index);
    } else {
        emit_two_bytes(get_op, index);
    }
}

static void and(bool can_assign) {
    (void )can_assign;
    int to_end = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);

    parse_precedence(PREC_AND);
    patch_jump(to_end);
}

static void or(bool can_assign) {
    (void )can_assign;
    int to_end = emit_jump(OP_JUMP_IF_TRUE);
    emit_byte(OP_POP);

    // 这里（包括上面的and）parse_precedence的优先级+1也可以，但要慢得多。
    // 如果+1，那么这个解析是不贪婪的，但本函数的调用者parse_precedence仍然会通过while循环解析后面的or
    // 如此一来，如果是 `1 or 2 or 3`的表达式，就会变成`(1 or 2) or 3`.
    // 该语句会被计算两次：第一次得到(1 or 2)为true，第二次得到`(1 or 2) or 3`为true。
    // 如果没有+1，那么这个解析本身是贪婪的，`1 or 2 or 3`会变成`1 or (2 or 3)`
    // 这种情况下，一旦知道1是true，后面的`(2 or 3)`就不需要判断了
    parse_precedence(PREC_OR );
    patch_jump(to_end);
}

/**
 * 从左到右的二元表达式的解析函数。
 * 在左侧操作项已经被解析, previous 为操作符时，向后解析并完成这个 binary 表达式。
 * 这个解析本身并不贪婪。
 *
 */
static void binary(bool can_assign) {
    (void )can_assign;
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

/**
 * 一元操作符表达式
 */
static void unary(bool can_assign) {
    (void )can_assign;
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

static inline void float_num(bool can_assign) {
    (void )can_assign;
    double value = strtod(parser.previous.start, NULL);
    int index = make_constant(float_value(value));
    emit_constant(index);
}

static inline void int_num(bool can_assign) {
    (void )can_assign;
    int value = (int) strtol(parser.previous.start, NULL, 10);
    int index = make_constant(int_value(value));
    emit_constant(index);
}

static void literal(bool can_assign) {
    (void )can_assign;
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

static inline void grouping(bool can_assign) {
    (void )can_assign;
    expression();
    consume(TOKEN_RIGHT_PAREN, "missing expected )");
}

/**
 * 该函数包装了 add_constant
 * 如果value属于预先装载值，那么直接返回其对应的索引。
 * 如果value属于String，如果该值已存在于lexeme_table中，则返回其索引。
 * 如果不存在，则用add_constant添加后，也将其存入lexeme_table中。
 * 若都不是，则将指定的 value 作为常数储存到 vm.code.constants 之中，然后返回其索引.
 * @return 常数的索引
 * */
static int make_constant(Value value) {
    int cache = constant_mapping(value);
    if (cache != -1) {
        return cache;
    }
    if (is_ref_of(value, OBJ_STRING)) {
        Value str_index;
        if (table_get(&parser.lexeme_table, as_string(value), &str_index) ) {
            return as_int(str_index);
        } else {
            str_index = int_value(add_constant(current_chunk(), value));
            table_set(&parser.lexeme_table, as_string(value), str_index);
            return as_int(str_index);
        }
    }
    int index = add_constant(current_chunk(), value);
    return index;
}

static void end_compiler() {
    emit_byte(OP_RETURN);
    if (SHOW_COMPILE_RESULT && !parser.has_error) {
        disassemble_chunk(current_chunk(), "disassembling the compiling result");
    }
}

static inline void emit_two_bytes(uint8_t byte1, uint8_t byte2) {
    emit_byte(byte1);
    emit_byte(byte2);
}

/**
 * 给定一个uint16的值，emit两个对应的uint8：i0，i1。
 * [7, 0]为i0，[15, 8]为i1
 * @param value
 */
static inline void emit_uint16(uint16_t value) {
    uint8_t i0, i1;
    u16_to_u8(value, &i0, &i1);
    // uint8_t i0 = value & 0xff;
    // uint8_t i1 = (value >> 8) & 0xff;
    emit_two_bytes(i0, i1);
}

/**
 * 根据给定的index产生OP_CONSTANT或者OP_CONSTANT2指令
 * @param index
 */
static void emit_constant(int index) {
    if (index < UINT8_MAX) {
        emit_two_bytes(OP_CONSTANT, index);
    } else if (index < UINT16_MAX) {
        emit_byte(OP_CONSTANT2);
        emit_uint16(index);
    } else {
        error_at_previous("Too many constants for a chunk");
    }
}

static Chunk *current_chunk() {
    return compiling_chunk;
}

static inline void emit_byte(uint8_t byte) {
    write_chunk(current_chunk(), byte, parser.previous.line);
}

/**
 * 判断curr是否是指定的type，如果是，移动，使它成为成为prev
 * @param type
 * @return 是否匹配
 */
static inline bool match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    } else {
        return false;
    }
}

static inline bool check(TokenType type) {
    return parser.current.type == type;
}

/**
 * 判断curr是否是指定的type，如果是，移动，使它成为prev。如果类型不匹配，则出现解析错误。
 * 消费之后，指定类型的那个token成为prev
 * @param type 指定类型
 * @param message 错误消息
 */
static inline void consume(TokenType type, const char *message) {
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

/**
 * 遇到解析错误后，试图移动到下一个可能正确的节点，继续解析。
 * 节点有两种：
 * 1. 以`;`标志的语句结束
 * 2. 以`var`, `fun`, `class`等关键字标志的语句的开始。
 */
static void synchronize() {
    parser.panic_mode = false;
    while (!check(TOKEN_EOF)) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                /* nothing */
                ;
        }
        advance();
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
        fprintf(stderr, " at [ %.*s ]", token->length, token->start);
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

static void init_scope(Scope *scope) {
    scope->depth = 0;
    scope->count = 0;
    current_scope = scope;
}

/**
 * 将目标源代码编译成字节码，写入到目标 chunk 中
 * */
bool compile(const char *src, Chunk *chunk) {

    init_scanner(src);
    init_table(&parser.lexeme_table);
    parser.continue_point = -1;
    parser.break_point = -1;
    compiling_chunk = chunk;
    Scope scope;
    init_scope(&scope);

    advance(); // 初始移动，如此一来，prev为null，curr为第一个token

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    end_compiler();
    bool has_error = parser.has_error;
    parser.panic_mode = false;
    parser.has_error = false;
    free_table(&parser.lexeme_table);
    return !has_error;
}
