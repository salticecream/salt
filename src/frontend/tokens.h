/* disclaimer:
this file is part of my salt project, which uses the gnu gplv3 license
*/
#pragma once

#include <string>
#include <iostream>
#include "../common.h"

#define TOKEN_STR_NAN "nan"
#define TOKEN_STR_NULL "null"
#define TOKEN_STR_TRUE "true"
#define TOKEN_STR_FALSE "false"
#define TOKEN_STR_INF "inf"

extern int lexer_line;
extern int lexer_col;

enum Token_e {
    // never use
    TOK_MIN         = -3,

    // bad token
    TOK_ERROR       = -2,

    // end of file
    TOK_EOF         = -1,
    // Only tokens > 0 should ever be generated by the lexer except for EOF,
    // which should only be generated once and at the end of the file.

    // The token numbers have been stripped from this source file since they
    // don't matter anyway, and just make it harder for me to add new tokens
    // where they belong.


    // no token
    // usually returned when lexer encounters tokens that changes its state (' and ")
    TOK_NONE        = 0,

    // commands
    TOK_FN,
    TOK_EXTERN,
    TOK_STRUCT,


    // other
    TOK_IDENT,              // identifier
    TOK_NUMBER,             // integer or floating point. count = 0 -> int, count = 1 -> float
    TOK_CHAR,               // single quotes
    TOK_STRING,             // in double quotes
    TOK_PTR,                // TOK_TYPE + any number of TOK_MUL
    TOK_LET,                // c++'s "auto"
    TOK_TYPE,               // like "int"
    TOK_WHITESPACE,         // like " "
    TOK_TAB,                // like "   "
    TOK_EOL,                // end of line
    TOK_EOS,                // end of statement
    TOK_MUT,
    TOK_CONST,              // possible dontuse; variables const by default
    
    // operators
    TOK_ADD,                // +
    TOK_SUB,                // -
    TOK_MUL,                // *
    TOK_DIV,                // /
    TOK_MODULO,             // %
    TOK_EXCLAMATION,        // !
    TOK_AMPERSAND,          // &
    TOK_VERTICAL_BAR,       // |
    TOK_TILDE,              // ~
    TOK_CARAT,              // ^
    TOK_INCREMENT,          // ++
    TOK_DECREMENT,          // --
    TOK_AS,                 // as

    // brackets
    TOK_LEFT_ANGLE,         // <
    TOK_RIGHT_ANGLE,        // >
    TOK_LEFT_SQUARE,        // [
    TOK_RIGHT_SQUARE,       // ]
    TOK_LEFT_BRACKET,       // (
    TOK_RIGHT_BRACKET,      // )

    // more other
    TOK_ASSIGN,             // = (assignment)
    TOK_COLON,              // :
    TOK_COMMA,              // ,
    TOK_DOT,                // .

    // equality
    TOK_EQUALS,             // ==
    TOK_NOT_EQUALS,         // !=
    TOK_EQUALS_LARGER,      // >=
    TOK_EQUALS_SMALLER,     // <=


    // more compound tokens
    TOK_ARROW,              // -> (compound token: TOK_SUB + TOK_RIGHT_ANGLE)
    TOK_LEFT_SHIFT,         // <<
    TOK_RIGHT_SHIFT,        // >>
    TOK_LEFT_SHIFT_ASSIGN,  // <<=
    TOK_RIGHT_SHIFT_ASSIGN, // >>=
    TOK_ADD_ASSIGN,         // +=
    TOK_SUB_ASSIGN,         // -=
    TOK_MUL_ASSIGN,         // *=
    TOK_DIV_ASSIGN,         // /=
    TOK_MODULO_ASSIGN,      // %=
    TOK_AND_ASSIGN,         // &=
    TOK_OR_ASSIGN,          // |=
    TOK_TILDE_ASSIGN,       // ~=
    TOK_XOR_ASSIGN,         // ^=
    
    // ctrl flow
    TOK_IF,
    TOK_ELSE,
    TOK_THEN,
    TOK_REPEAT,
    TOK_RETURN,

    /* To be implemented much later
    TOK_SWITCH,
    TOK_CASE,
    TOK_DEFAULT,
    */

    // boolean
    TOK_AND,                // && or and
    TOK_OR,                 // || or or
    TOK_NOT,                // TOK_EXCLAMATION generally decays to this

    // Comments; implement these later pls :D
    TOK_LINE_COMMENT,       // //
    TOK_COMMENT_START,      // /*
    TOK_COMMENT_END,        // */


    // default types, should not be pushed to the Lexer's vector
    TOK_VOID,
    TOK_BOOL,
    TOK_CHAR_TYPE,
    TOK_SHORT,
    TOK_INT,
    TOK_LONG,
    TOK_SSIZE,
    TOK_FLOAT,
    TOK_DOUBLE,
    TOK_UCHAR,
    TOK_USHORT,
    TOK_UINT,
    TOK_ULONG,
    TOK_USIZE,
    TOK_UNSIGNED,

    // constants
    TOK_TRUE,
    TOK_FALSE,
    TOK_NULL,
    TOK_INF,
    TOK_NAN,

    
    TOK_TOTAL
}; 

class Token {
private:

    Token_e val_;
    std::string data_;
    int count_;
    int line_;
    int col_;

public:

    Token(Token_e value);
    Token(Token_e value, const std::string& data);
    Token(Token_e value, const std::string&& data);
    Token(Token_e value, const std::string& data, int count);
    Token(Token_e value, const std::string&& data, int count);
    Token(Token_e value, const std::string& data, int count, int line, int col);

    explicit operator int() const;
    explicit operator bool() const;

    // Converts a token to a string.
    std::string str() const;

    // Retrieves the data from a string, if there is any.
    std::string& data() const;
    int count() const;

    bool has_data() const;

    Token_e val() const;

    int col() const;
    int line() const;

    // Make the Token printable with std::cout and std::cerr
    friend std::ostream& operator<<(std::ostream& os, const Token& token);

    // Convert the token to a pointer if possible
    Token as_pointer() const;

    bool is_whitespace() const;
};

// An exception that is thrown when a bad char is encountered.
class BadCharException : public salt::Exception {
public:
    using Exception::Exception;
        
};