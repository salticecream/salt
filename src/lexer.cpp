#include "lexer.h"
#include <iostream>
#include <vector>

using std::vector;

Lexer* Lexer::instance = nullptr;
/* private: */

// Constructs a new Lexer.
Lexer::Lexer() {
    this->_col = 1;
    this->_line = 1;
    Lexer::instance = this;
}


/* public: */

// Returns a pointer to the current Lexer instance.
inline Lexer* Lexer::get() {
    return instance ? instance : new Lexer();
}

// Destroys the current lexer. Probably doesn't need to be called, since the
// lexer should last for the entire program anyway.
inline void Lexer::destroy() {
    if (auto lexer = Lexer::get()) {
        Lexer::instance = nullptr;
        delete lexer;
    }
}

// Returns the current line of the lexer.
inline int Lexer::line() const {
    return this->_line;
}

// Returns the current column of the lexer.
inline int Lexer::col() const {
    return this->_col;
}

// Returns the amount of chars read in total by the lexer.
inline int Lexer::pos() const {
    return this->_pos;
}

Token Lexer::next_token() {
    int res = 0;

    return Token(res);
}

void tokenize(const char* file_name, vector<Token>& vec) {
    std::cerr << "warning: next_token() NYI" << std::endl;
    Lexer* lexer = Lexer::get();
    while (const Token next = lexer->next_token())
        vec.push_back(next);
}