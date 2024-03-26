#pragma once
#include <map>
#include "tokens.h"


namespace BinaryOperator {
	extern std::map<Token_e, int> BINOP_MAP;
	void fill_map();
	int get_precedence(const Token_e val);
}