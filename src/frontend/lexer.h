#pragma once

#include "tokens.h"
#include "miniregex.h"
#include "../common.h"
#include <vector>

enum LexerState {
    LEXER_STATE_NORMAL,
    LEXER_STATE_CHAR,
    LEXER_STATE_STRING,
    LEXER_STATE_LINE_COMMENT,
    LEXER_STATE_BLOCK_COMMENT
};

class Lexer {
private:
    int col_;
    int line_;
    int pos_;
    LexerState state_;
    static Lexer* instance_;
    std::vector<salt::Exception> errors_;
    Lexer();

public:
    static Lexer* get();
    static void destroy();


    salt::Result<Token> next_token();
    Token end_token();
    Lexer* test_lexer();
    int col() const;
    int line() const;
    int pos() const;
    LexerState state() const;
    std::string current_string;
    std::string identifier_string;
    int current_number;
    const std::vector<salt::Exception>& errors();

    // don't use Lexer::tokenize(), use simple tokenize() instead
    std::vector<Token> tokenize(const char* str);

};

// Parses the input file into tokens, which are stored in vec.
std::vector<Token> tokenize(const char* str);
