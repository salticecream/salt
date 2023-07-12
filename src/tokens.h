/* disclaimer:
this file is part of my salt project, which uses the gnu gplv3 license
*/
#ifndef SALT_TOKENS_H
#define SALT_TOKENS_H
#include <string>

enum Token_e {
    // end of file
    TOK_EOF = 0,

    // commands
    TOK_FN = 1,
    TOK_EXTERN = 2,
    TOK_STRUCT = 3,

    // (will not be implemented atm) keywords

    // other
    TOK_IDENT = 4,
    TOK_NUMBER = 5, // only int
    TOK_PTR = 6, // only void*


    // (will not be implemented atm) types
    
};

extern std::string ident_str;
extern int num_val;
extern const char ALLOWED_CHARS[];
inline bool allowed(const char ch);

class Token {
private:

    int val;

public:

    Token(int value);

    explicit operator int() const;
    operator bool() const;
};



#endif