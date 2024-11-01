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

enum class LexerInputMode {
    LEXER_INPUT_MODE_STDIN,
    LEXER_INPUT_MODE_FILE
};

class Lexer {
private:
    int col_;
    int line_;
    int pos_;
    std::vector<Token> vec;
    LexerState state_;
    static Lexer* instance_;
    std::vector<salt::Exception> errors_;
    std::unique_ptr<std::ifstream> stream_;
    int next_char();
    LexerInputMode input_mode_;
    bool eof_reached = false;
    Lexer();
    ~Lexer();

public:
    static Lexer* get();
    static void destroy();


    salt::Result<Token> next_token();
    Token end_token();
    Token& last_non_whitespace();
    Lexer* test_lexer();
    void set_input_mode(LexerInputMode input_mode, std::unique_ptr<std::ifstream> stream);
    LexerInputMode input_mode() const;
    int col() const;
    int line() const;
    int pos() const;
    LexerState state() const;


    std::string current_string; /// @todo: make these 2 private
    std::string identifier_string;

    const std::vector<salt::Exception>& errors();

    // don't use Lexer::tokenize(), use simple tokenize() instead
    std::vector<Token>& tokenize(const char* str = nullptr);

};

// Parses the input file into tokens, which are stored in vec.
std::vector<Token>& tokenize(const char* str);
