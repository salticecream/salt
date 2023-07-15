#ifndef SALT_LEXER_H
#define SALT_LEXER_H
#include "tokens.h"
#include "miniregex.h"
#include <vector>

class Lexer {
private:
    int _col;
    int _line;
    int _pos;
    static Lexer* instance;
    Lexer();

public:
    static Lexer* get();
    static void destroy();


    int next_tokens(std::vector<Token>& vec);
    int col() const;
    int line() const;
    int pos() const;
    std::string current_string;
    std::string identifier_string;
    int current_number;
    int state;

};

// Parses the input file into tokens, which are stored in vec.
int tokenize(const char* str, std::vector<Token>& vec);


// Like a Rust enum but worse.
class LexerState {
private:
    static const int AS_TOKEN_VALID_INTS[];

public:
    static const int TOK_INVALID_LEXER_STATE = -2;
    static const int NONE = 0;
    static const int FN = 1;
    static const int EXTERN = 2;
    static const int STRUCT = 3;
    static const int NUMBER = 4;
    static const int LET = 5;
    static const int TOTAL = 6;

    // Converts a keyword-related LexerState to its corresponding token.
    // If an invalid LexerState is received, the function returns a "-2" token.
    static Token as_token(const int x);

};

#endif