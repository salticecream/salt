#pragma once
#include <map>
#include "tokens.h"
extern std::map<Token_e, int> BINOP_MAP;

namespace BinaryOperator {
void fill_map();
int get_precedence(const Token_e val);
}