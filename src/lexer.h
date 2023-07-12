#ifndef SALT_LEXER_H
#define SALT_LEXER_H
#include "tokens.h"
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


    Token next_token();
    int col() const;
    int line() const;
    int pos() const;

};

// Parses the input file into tokens, which are stored in vec.
void tokenize(const char* file, std::vector<Token>& vec);

#endif