#include "lexer.h"
#include <iostream>
#include <vector>

using std::vector, std::string;

Lexer* Lexer::instance = nullptr;
/* private: */

// Constructs a new Lexer.
Lexer::Lexer() {
    this->_col = 1;
    this->_line = 1;
    Lexer::instance = this;
    this->current_string = std::string();
    this->identifier_string = std::string();
    this->current_number = 0;
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

const int LexerState::AS_TOKEN_VALID_INTS[] = {
    LexerState::FN,
    LexerState::EXTERN,
    LexerState::STRUCT,
    LexerState::NUMBER,
    LexerState::LET,
};

Token LexerState::as_token(const int n) {
    for (int valid : LexerState::AS_TOKEN_VALID_INTS) {
        if (n == valid)
            return Token(n);
    }
        return Token(LexerState::TOK_INVALID_LEXER_STATE);
}

// Appends the next tokens to the vector.
// Returns the amount of tokens appended, or 0 at EOF.
/// @todo Refactor this so it follows a more reasonable style.
int Lexer::next_tokens(vector<Token>& vec) {
    Lexer* lexer = Lexer::get();
    int res = 0;
    string string_res;

    while (const char ch = getchar()) {
        if (ch == EOF || !ch) {
            vec.push_back(Token(TOK_EOF));
            return 0;
        }

        // Whitespaces end the token, so in the following lines we will check
        // which token we are dealing with.
        if (is_whitespace(ch)) {

            // If the whitespace is a newline, add that too.
            if (ch == '\n') {
                vec.push_back(Token(TOK_EOL));
                res++;
            }

            // Now add the correct token.
            if (string_res == "fn") {
                vec.push_back(Token(TOK_FN));
                string_res.clear();
                res++;

            } else if (string_res == "extern") {
                vec.push_back(Token(TOK_EXTERN));
                string_res.clear();
                res++;

            } else if (string_res == "struct") {
                vec.push_back(Token(TOK_STRUCT));
                string_res.clear();
                res++;

            } else if (string_res == "let") {
                vec.push_back(Token(TOK_LET));
                string_res.clear();
                res++;

            } else if (is_integer(string_res.c_str())) {
                vec.push_back(Token(TOK_NUMBER, string_res));
                string_res.clear();
                res++;
            } else if (is_type(string_res)) {
                vec.push_back(Token(TOK_TYPE, string_res));
                string_res.clear();
                res++;    

            } else {
                vec.push_back(Token(TOK_IDENT, string_res));
                string_res.clear();
                res++;
            }

            return res;

        } else { 
        // the current char is not a whitespace, add it to string_res
            string_res += ch;

        }
    }
    return res;
}

int tokenize(const char* str, vector<Token>& vec) {
    int res = 0;
    Lexer* lexer = Lexer::get();

    while (const int new_tokens_count = lexer->next_tokens(vec))
        res += new_tokens_count;

    for (const Token& tok : vec)
        std::cout << tok << std::endl;

    return res;
}