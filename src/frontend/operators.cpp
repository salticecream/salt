#include "operators.h"

static inline int constexpr millions(int a) {return a * 1000000;}

namespace BinaryOperator {

    std::map<Token_e, int> BINOP_MAP;

    void fill_map() {
        salt::dboutv << "Filling map of binary operators\n";
        BINOP_MAP[TOK_AS] = millions(22);

        BINOP_MAP[TOK_MUL] = millions(21);
        BINOP_MAP[TOK_DIV] = BINOP_MAP[TOK_MUL];

        BINOP_MAP[TOK_ADD] = millions(20);
        BINOP_MAP[TOK_SUB] = BINOP_MAP[TOK_ADD];

        BINOP_MAP[TOK_LEFT_SHIFT] = millions(19);
        BINOP_MAP[TOK_RIGHT_SHIFT] = BINOP_MAP[TOK_LEFT_SHIFT];

        BINOP_MAP[TOK_LEFT_ANGLE] = millions(18);
        BINOP_MAP[TOK_RIGHT_ANGLE] = BINOP_MAP[TOK_LEFT_ANGLE];
        BINOP_MAP[TOK_EQUALS_SMALLER] = BINOP_MAP[TOK_LEFT_ANGLE];
        BINOP_MAP[TOK_EQUALS_LARGER] = BINOP_MAP[TOK_LEFT_ANGLE];

        BINOP_MAP[TOK_EQUALS] = millions(17);
        BINOP_MAP[TOK_NOT_EQUALS] = BINOP_MAP[TOK_EQUALS];

        // Bitwise AND
        BINOP_MAP[TOK_AMPERSAND] = millions(16);
        
        // Bitwise XOR
        BINOP_MAP[TOK_CARAT] = millions(15);

        // Bitwise OR
        BINOP_MAP[TOK_VERTICAL_BAR] = millions(14);

        // && or "and"
        BINOP_MAP[TOK_AND] = millions(13);

        // || or "or"
        BINOP_MAP[TOK_OR] = millions(12);

        BINOP_MAP[TOK_ASSIGN] = millions(11);
        BINOP_MAP[TOK_ADD_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_SUB_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_MUL_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_DIV_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_MODULO_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_LEFT_SHIFT_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_RIGHT_SHIFT_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_AND_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_OR_ASSIGN] = TOK_ASSIGN;
        BINOP_MAP[TOK_XOR_ASSIGN] = TOK_ASSIGN;
    }   

    int get_precedence(const Token_e val) {
        if (int res = BINOP_MAP[val])
            return res;
        return -1;
    }

}