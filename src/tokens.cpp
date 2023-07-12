/* disclaimer:
this file is part of my salt project, which uses the gnu gplv3 license
*/

#include "tokens.h"
#include <string>
static const int SALT_TOKENS_ALLOWED_CHARS = 68;
static std::string ident_str;
static int num_val;

static const char ALLOWED_CHARS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"@#%&/()=+-*<>.,:;\n ";

inline bool allowed(const char ch) {
    for (int i = 0; i < SALT_TOKENS_ALLOWED_CHARS; i++)
        if (ch == ALLOWED_CHARS[i])
            return true;
        return false;
}

Token::Token(int value) {
    this->val = value;
}

Token::operator int() const {
    return this->val;
}

Token::operator bool() const {
    return this->val != 0;
}