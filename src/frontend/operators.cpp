#include "operators.h"

std::map<Token_e, int> BINOP_MAP;

static inline int constexpr millions(int a) {return a * 1000000;}

namespace BinaryOperator {

void fill_map() {
    BINOP_MAP[TOK_MUL] = millions(21);
    BINOP_MAP[TOK_DIV] = BINOP_MAP[TOK_MUL];
    BINOP_MAP[TOK_ADD] = millions(20);
    BINOP_MAP[TOK_SUB] = BINOP_MAP[TOK_ADD];
    BINOP_MAP[TOK_LEFT_SHIFT] = millions(19);
    BINOP_MAP[TOK_RIGHT_SHIFT] = BINOP_MAP[TOK_LEFT_SHIFT];
    BINOP_MAP[TOK_LEFT_ANGLE] = millions(1);
    BINOP_MAP[TOK_RIGHT_ANGLE] = BINOP_MAP[TOK_LEFT_ANGLE];
}

int get_precedence(const Token_e val) {
    switch (val) {
    case TOK_ADD:
    case TOK_SUB:
    case TOK_MUL:
    case TOK_DIV:
    case TOK_LEFT_SHIFT:
    case TOK_RIGHT_SHIFT:
    case TOK_LEFT_ANGLE:
    case TOK_RIGHT_ANGLE:
        return BINOP_MAP[val];
    default:
        return -1;
    }
}

}