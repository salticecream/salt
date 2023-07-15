/* disclaimer:
this file is part of my salt project, which uses the gnu gplv3 license
*/

#include "tokens.h"
#include <string>
Token::Token(int value) {
    this->val = value;
    this->_data = std::string("");
}

Token::Token(int value, std::string data) {
    this->val = value;
    this->_data = data;
}

std::string& Token::data() const {
    return const_cast<std::string&>(this->_data);
}

Token::operator int() const {
    return this->val;
}

Token::operator bool() const {
    return this->val != 0;
}

bool Token::has_data() const {
    return this->data() != "";
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.str();
    if (token.has_data())
        os << " (" << token.data() << ')';
    return os;
}

std::string Token::str() const {
    switch (this->val) {
    case TOK_EOF:
        return "EOF";
    case TOK_EXTERN:
        return "EXTERN";
    case TOK_FN:
        return "FN";
    case TOK_IDENT:
        return "IDENT";
    case TOK_NUMBER:
        return "NUMBER";
    case TOK_PTR:
        return "PTR";
    case TOK_STRUCT:
        return "STRUCT";
    case TOK_LET:
        return "LET";
    case TOK_TYPE:
        return "TYPE";
    case TOK_EOL:
        return "EOL";
    case TOK_EOS:
        return "EOS";
    default:
        return std::string("error token (" + std::to_string(this->val) + ")");
    }
}