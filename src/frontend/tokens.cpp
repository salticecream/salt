/* disclaimer:
this file is part of my salt project, which uses the gnu gplv3 license
*/

#include "tokens.h"
#include <string>
Token::Token(Token_e value) {
    this->val_ = value;
    this->data_ = std::string("");
    this->count_ = 0;
    this->col_ = lexer_col;
    this->line_ = lexer_line;
}

Token::Token(Token_e value, const std::string& data) {
    this->val_ = value;
    this->data_ = data;
    this->count_ = 0;
    this->col_ = lexer_col;
    this->line_ = lexer_line;
}

Token::Token(Token_e value, const std::string&& data) {
    this->val_ = value;
    this->data_ = data;
    this->count_ = 1;
    this->col_ = lexer_col;
    this->line_ = lexer_line;
}

Token::Token(Token_e value, const std::string& data, int count) {
    this->val_ = value;
    this->data_ = data;
    this->count_ = count;
    this->col_ = lexer_col;
    this->line_ = lexer_line;
}

Token::Token(Token_e value, const std::string&& data, int count) {
    this->val_ = value;
    this->data_ = data;
    this->count_ = count;
    this->col_ = lexer_col;
    this->line_ = lexer_line;
}

Token::Token(Token_e value, const std::string& data, int count, int line, int col) {
    this->val_ = value;
    this->data_ = data;
    this->count_ = count;
    this->col_ = col;
    this->line_ = line;
}

std::string& Token::data() const {
    return const_cast<std::string&>(this->data_);
}

int Token::col() const {
    return this->col_;
}

int Token::line() const {
    return this->line_;
}

Token::operator int() const {
    return this->val_;
}

Token::operator bool() const {
    return this->val_ != TOK_NONE;
}

bool Token::has_data() const {
    return this->data() != "";
}

int Token::count() const {
    return this->count_;
}

bool Token::is_whitespace() const {
    switch (val()) {
    case TOK_WHITESPACE:
    case TOK_TAB:
    case TOK_EOL:
        return true;
    default:
        return false;
    }
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    if (token.str().empty())
        os << "UNPRINTABLE TOKEN NUMBER " << int(token.val());
    else
        os << token.str();

    if (token.has_data())
        os << " (\"" << token.data() << "\")";
    
    int count = token.count();
    if (count > 0)
        os << " [" << count << ']';

    return os;
}

Token_e Token::val() const {
    return this->val_;
}

std::string Token::str() const {
    switch (this->val()) {
    case TOK_ERROR:
        return std::string("ERROR_TOKEN");
    case TOK_EOF:
        return "EOF";
    case TOK_FN:
        return "FN";
    case TOK_EXTERN:
        return "EXTERN";
    case TOK_STRUCT:
        return "STRUCT";
    case TOK_IDENT:
        return "IDENT";
    case TOK_NUMBER:
        return "NUMBER";
    case TOK_CHAR:
        return "CHAR";
    case TOK_STRING:
        return "STRING";
    case TOK_PTR:
        return "PTR";
    case TOK_LET:
        return "LET";
    case TOK_TYPE:
        return "TYPE";
    case TOK_WHITESPACE:
        return "WHITESPACE";
    case TOK_TAB:
        return "TAB";
    case TOK_EOL:
        return "EOL";
    case TOK_EOS:
        return "EOS";
    case TOK_MUT:
        return "MUT";
    case TOK_CONST:
        return "CONST";

    case TOK_ADD:
        return "ADD";
    case TOK_SUB:
        return "SUB";
    case TOK_MUL:
        return "MUL";
    case TOK_DIV:
        return "DIV";
    case TOK_MODULO:
        return "MODULO";
    case TOK_EXCLAMATION:
        return "EXCLAMATION";
    case TOK_AMPERSAND:
        return "AMPERSAND";
    case TOK_VERTICAL_BAR:
        return "VERTICAL_BAR";
    case TOK_TILDE:
        return "TILDE";
    case TOK_CARAT:
        return "CARAT";
    case TOK_INCREMENT:
        return "INCREMENT";
    case TOK_DECREMENT:
        return "DECREMENT";
    case TOK_AS:
        return "AS";
    case TOK_LEFT_ANGLE:
        return "LEFT_ANGLE";
    case TOK_RIGHT_ANGLE:
        return "RIGHT_ANGLE";
    case TOK_LEFT_SQUARE:
        return "LEFT_SQUARE";
    case TOK_RIGHT_SQUARE:
        return "RIGHT_SQUARE";
    case TOK_LEFT_BRACKET:
        return "LEFT_BRACKET";
    case TOK_RIGHT_BRACKET:
        return "RIGHT_BRACKET";
    case TOK_ASSIGN:
        return "ASSIGN";
    case TOK_COLON:
        return "COLON";
    case TOK_COMMA:
        return "COMMA";
    case TOK_DOT:
        return "DOT";
    case TOK_EQUALS:
        return "EQUALS";
    case TOK_NOT_EQUALS:
        return "NOT_EQUALS";
    case TOK_EQUALS_LARGER:
        return "EQUALS_LARGER";
    case TOK_EQUALS_SMALLER:
        return "EQUALS_SMALLER";
    case TOK_ARROW:
        return "ARROW";
    case TOK_LEFT_SHIFT:
        return "LEFT_SHIFT";
    case TOK_RIGHT_SHIFT:
        return "RIGHT_SHIFT";
    case TOK_LEFT_SHIFT_ASSIGN:
        return "LEFT_SHIFT_ASSIGN";
    case TOK_RIGHT_SHIFT_ASSIGN:
        return "RIGHT_SHIFT_ASSIGN";
    case TOK_ADD_ASSIGN:
        return "ADD_ASSIGN";
    case TOK_SUB_ASSIGN:
        return "SUB_ASSIGN";
    case TOK_MUL_ASSIGN:
        return "MUL_ASSIGN";
    case TOK_DIV_ASSIGN:
        return "DIV_ASSIGN";
    case TOK_MODULO_ASSIGN:
        return "MODULO_ASSIGN";
    case TOK_AND_ASSIGN:
        return "AND_ASSIGN";
    case TOK_OR_ASSIGN:
        return "OR_ASSIGN";
    case TOK_TILDE_ASSIGN:
        return "TILDE_ASSIGN";
    case TOK_XOR_ASSIGN:
        return "XOR_ASSIGN";
    case TOK_IF:
        return "IF";
    case TOK_ELSE:
        return "ELSE";
    case TOK_THEN:
        return "THEN";
    case TOK_REPEAT:
        return "REPEAT";
    case TOK_RETURN:
        return "RETURN";
    case TOK_AND:
        return "AND";
    case TOK_OR:
        return "OR";
    case TOK_NOT:
        return "NOT";
    case TOK_LINE_COMMENT:
        return "LINE_COMMENT";
    case TOK_COMMENT_START:
        return "COMMENT_START";
    case TOK_COMMENT_END:
        return "COMMENT_END";
    case TOK_VOID:
        return "VOID";
    case TOK_BOOL:
        return "BOOL";
    case TOK_CHAR_TYPE:
        return "CHAR_TYPE";
    case TOK_SHORT:
        return "SHORT";
    case TOK_INT:
        return "INT";
    case TOK_LONG:
        return "LONG";
    case TOK_SSIZE:
        return "SSIZE";
    case TOK_FLOAT:
        return "FLOAT";
    case TOK_DOUBLE:
        return "DOUBLE";
    case TOK_UCHAR:
        return "UCHAR";
    case TOK_USHORT:
        return "USHORT";
    case TOK_UINT:
        return "UINT";
    case TOK_ULONG:
        return "ULONG";
    case TOK_USIZE:
        return "USIZE";
    case TOK_UNSIGNED:
        return "UNSIGNED";
    case TOK_TRUE:
        return "TRUE";
    case TOK_FALSE:
        return "FALSE";
    case TOK_NULL:
        return "NULL";
    case TOK_INF:
        return "INF";
    case TOK_NAN:
        return "NAN";
        
    default:
        if (TOK_MIN < this->val() && this->val() < TOK_TOTAL)
            return "VALID UNKNOWN " + std::to_string(this->val());
        else
            return std::string("INVALID ") + std::to_string(this->val());
    }
}


[[deprecated("TOK_PTR is deprecated")]]
Token Token::as_pointer() const {
    if (this->val() != TOK_TYPE)
        salt::print_fatal(salt::Exception((std::to_string(line_) + ':' + std::to_string(col_) + ':' + "(internal) Bad call to Token::to_pointer()").c_str()));
    
    return Token(TOK_PTR, this->data());
}