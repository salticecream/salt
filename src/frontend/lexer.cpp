#include "lexer.h"
#include "../common.h"
#include <iostream>
#include <vector>

using salt::Result, salt::Result_e;

Lexer* Lexer::instance_ = nullptr;
int lexer_col = 1;
int lexer_line = 1;
/* private: */

// Constructs a new Lexer.
// This is done in Lexer::tokenize().

Lexer::Lexer() {
    this->col_ = 1;
    this->line_ = 1;
    this->pos_ = 1;
    this->current_string = std::string();
    this->identifier_string = std::string();
    this->state_ = LexerState::LEXER_STATE_NORMAL;
    this->errors_ = std::vector<salt::Exception>();
    this->stream_ = nullptr;
    this->input_mode_ = LexerInputMode::LEXER_INPUT_MODE_STDIN;
}

Lexer::~Lexer() {
    if (stream_) {
        stream_->close();
        stream_ = nullptr;
    }
}


/* public: */

// Returns a pointer to the current Lexer instance.
Lexer* Lexer::get() {
    return instance_ ? instance_ : instance_ = new Lexer();
}

// Destroys the current lexer. Probably doesn't need to be called, since the
// lexer should last for the entire program anyway.
void Lexer::destroy() {
    if (Lexer* lexer = Lexer::get()) {
        delete lexer;
        Lexer::instance_ = nullptr;
    }
}

// Returns the current line of the lexer.
inline int Lexer::line() const {
    return this->line_;
}

// Returns the current column of the lexer.
inline int Lexer::col() const {
    return this->col_;
}

// Returns the amount of chars read in total by the lexer.
inline int Lexer::pos() const {
    return this->pos_;
}

inline LexerState Lexer::state() const {
    return this->state_;
}

// only for use in testing, might be moved to testing/testing.h
inline Lexer* Lexer::test_lexer() {
    return new Lexer();
}

const std::vector<salt::Exception>& Lexer::errors() {
    return errors_;
}

void Lexer::set_input_mode(LexerInputMode input_mode, std::unique_ptr<std::ifstream> stream) {
    input_mode_ = input_mode;
    stream_ = std::move(stream);
}

LexerInputMode Lexer::input_mode() const {
    return input_mode_;
}



int Lexer::next_char() {
    int res = 0;
    if (input_mode_ == LexerInputMode::LEXER_INPUT_MODE_STDIN)
        res = std::cin.get();
    else if (input_mode_ == LexerInputMode::LEXER_INPUT_MODE_FILE)
        res = stream_->get();
    else {
        res = 0;
        any_compile_error_occured = true;
        salt::print_fatal("bad LexerInputMode");
    }
    if (res == EOF)
        eof_reached = true;
    
    return res;
}

// Helper function for next_token(). Returns the token that corresponds
// to whatever the lexer's current_string is. Additionally trims the 
// lexer's current_string to its last whitespace.
// Note: string and char literals are handled differently.
[[nodiscard]]
Token Lexer::end_token() {

    // Check for 1-char tokens, such as \n or +
    if (this->current_string.size() == 1) {
        const char temp_ch = this->current_string[0];
        // First make sure the token is a symbol, otherwise we handle it like a
        // multi-char token (such as an identifier)
        if (is_alphanumeric(temp_ch)) { // equivalent to !is_valid_symbol() here
            ;
        } else {
            this->current_string.clear();
            switch (temp_ch) {
            case EOF:
                // this only happens when compiling a file
                // instead of writing it directly using this program
                // we need to return NONE, since we are inserting an EOF into the vector ourselves at the end 
                return Token(TOK_EOF);
            case '+':
                return Token(TOK_ADD);
            case '-':
                return Token(TOK_SUB);
            case '*':
                return Token(TOK_MUL);
            case '/':
                return Token(TOK_DIV);
            case ' ':
                return Token(TOK_WHITESPACE);
            case '\t':
                return Token(TOK_TAB);
            case '\n':
                return Token(TOK_EOL);
            case '!':
                return Token(TOK_EXCLAMATION);
            case '&':
                return Token(TOK_AMPERSAND);
            case '|':
                return Token(TOK_VERTICAL_BAR);
            case '~':
                return Token(TOK_TILDE);
            case '<':
                return Token(TOK_LEFT_ANGLE);
            case '>':
                return Token(TOK_RIGHT_ANGLE);
            case '=':
                return Token(TOK_ASSIGN);
            case '(':
                return Token(TOK_LEFT_BRACKET);
            case ')':
                return Token(TOK_RIGHT_BRACKET);
            case ':':
                return Token(TOK_COLON);
            case ',':
                return Token(TOK_COMMA);


            default:
                std::string str = std::string('\'' + std::to_string(temp_ch) + '\'');
                return Token(TOK_ERROR, std::to_string(int(temp_ch)));
            }
        }
    }

    Lexer* lexer = this;
    std::string string_res = trim_last(this->current_string);
    std::string& cur_str = this->current_string;

    // Keywords

    if (string_res == "fn") {
        lexer->current_string = cur_str.back();
        return Token(TOK_FN);

    } else if (string_res == "extern") {
        lexer->current_string = cur_str.back();
        return Token(TOK_EXTERN);

    } else if (string_res == "struct") {
        lexer->current_string = cur_str.back();
        return Token(TOK_STRUCT);

    } else if (string_res == "mut") {
        lexer->current_string = cur_str.back();
        return Token(TOK_MUT);

    } else if (string_res == "const") {
        lexer->current_string = cur_str.back();
        return Token(TOK_CONST);

    } else if (string_res == "let") {
        lexer->current_string = cur_str.back();
        return Token(TOK_LET);

    } else if (string_res == "if") {
        lexer->current_string = cur_str.back();
        return Token(TOK_IF);

    } else if (string_res == "else") {
        lexer->current_string = cur_str.back();
        return Token(TOK_ELSE);
    } else if (string_res == "and") {
        lexer->current_string = cur_str.back();
        return Token(TOK_AND);
    } else if (string_res == "not") {
        lexer->current_string = cur_str.back();
        return Token(TOK_NOT);
    } else if (string_res == "or") {
        lexer->current_string = cur_str.back();
        return Token(TOK_OR);

    } else if (string_res == "then") {
        lexer->current_string = cur_str.back();
        return Token(TOK_THEN);
    } else if (string_res == "repeat") {
        lexer->current_string = cur_str.back();
        return Token(TOK_REPEAT);

    

    // These tokens have data, so before clearing the string, 
    // we need to copy that data

    } else if (is_integer(string_res.c_str())) {
        std::string _data = string_res;
        lexer->current_string = cur_str.back();
        return Token(TOK_NUMBER, _data);
        
    } else if (is_type(string_res)) {
        std::string _data = string_res;
        lexer->current_string = cur_str.back();
        return Token(TOK_TYPE, _data); 

    } else if (!string_res.empty()){
        std::string _data = string_res;
        lexer->current_string = cur_str.back();
        return Token(TOK_IDENT, _data);

    } else {
        return Token(TOK_NONE);
    }
}

// Returns the next token from stdin.
Result<Token> Lexer::next_token() {

    std::string& cur_str = this->current_string;

    // If the string contains any data already, we return that as a token.
    if (!cur_str.empty())
        return this->end_token();

    switch (this->state()) {
    case LEXER_STATE_NORMAL:
        while (true) {
            const char ch = next_char();
            // Move the lexer's position forward.
            this->pos_++;
            this->col_++;
            lexer_col++;

            if (ch == '\n') {
                this->col_ = 1;
                lexer_col = 1;
                this->line_++;
                lexer_line++;
            }

            if (ch == '\'') {
                this->state_ = LEXER_STATE_CHAR;
                return Token(TOK_NONE);
            }

            if (ch == '"') {
                this->state_ = LEXER_STATE_STRING;
                return Token(TOK_NONE);
            }

            // We check if the char is an allowed symbol or an EOF.
            // (A null byte is not possible here since those break the loop)
            // If not, throw an error.
            if (!is_allowed(ch) && ch != EOF) {
                void(end_token()); // Discard the current token and just move forward
                any_compile_error_occured = true;
                return BadCharException(std::to_string(this->line()) + ':'
                    + std::to_string(this->col()) + ":" + " Invalid char: `" + ch + '`'
                    + "\nASCII: " + std::to_string(int(ch)));
            }

            // Add the char to the current string, since it's allowed.
            cur_str += ch;

            // If ch is a token separator, such as \n or +, we return the start
            // of the current string as a token (ignoring the last char which
            // we just added).
            if (is_separator(ch) || ch == EOF)
                return this->end_token();
        }

        return Token(TOK_ERROR, cur_str);


    // please only 2 cases in this bracket, otherwise everything breaks
    case LEXER_STATE_CHAR:
    case LEXER_STATE_STRING: {
        TODO();
        // Not handling escape characters for now
        char end_char = this->state() == LEXER_STATE_CHAR ? '\'' : '"';
        while (true) {
            if (pos_ > 10000)
                throw std::exception("too long file: 10000 bytes (defined in lexer.cpp:250)");
            const char ch_str = next_char();
            printf("Next_char() was: %d\n", ch_str);

            // Move the lexer's position forward.
            this->pos_++;
            this->col_++;
            lexer_col++;

            if (ch_str == '\n') {
                this->col_ = 1;
                this->line_++;
                lexer_col = 1;
                lexer_line++;
            }

            if (ch_str == EOF || ch_str == end_char) {
                std::string string_to_return = cur_str;
                cur_str.clear();
                Token_e token_val = this->state() == LEXER_STATE_CHAR ? TOK_CHAR : TOK_STRING;
                this->state_ = LEXER_STATE_NORMAL;
                return Token(token_val, string_to_return);
            }

            cur_str += ch_str;
        }
        break; // Not necessary because of while (true) loop above, but compiler is not smart enough to know this
    }
    default:
        return Token(TOK_ERROR, cur_str);
    }
}

std::vector<Token> Lexer::tokenize(const char* str) {

    Lexer* lexer = this;
    Lexer* me = Lexer::get();

    if (this != me) {
        any_compile_error_occured = true;
        salt::print_fatal(salt::f_string("Wrong address for lexer.\nThis: %p\n Lexer::get(): %p", this, me).c_str());
    }
    
    std::vector<Token> vec;

    if (lexer->current_string.size() > 0)
        std::cerr << "warning: lexer still has string data left before tokenize: "
            << lexer->current_string;

    lexer->current_string.clear();
    Token NO_TOKEN = Token(TOK_NONE);

    // if a string was passed into tokenize, try to read that as a file
    if (str) {

        if (!string_ends_with(str, ".sl")) {
            salt::print_fatal(std::string(str) + ": file must end in .sl");
        }

       
        std::unique_ptr<std::ifstream> new_stream = std::make_unique<std::ifstream>(str);
        if (!new_stream || !new_stream->is_open()) {
            salt::print_fatal(std::string(str) + ": could not open file");
        }

        salt::dbout << "compiling " << str << std::endl;
        set_input_mode(LexerInputMode::LEXER_INPUT_MODE_FILE, std::move(new_stream));
    }

    while (true) {
        if (Result<Token> next_res = lexer->next_token()) {
            const Token next = next_res.unwrap();
            Token& last = (vec.empty() ? NO_TOKEN : vec[vec.size() - 1]);



            //
            // Handling of compound tokens, and ignore "NONE" tokens.
            //

            switch (next.val()) {
            case TOK_NONE:
                break;

            // If a whitespace follows another whitespace, it counts as 1 whitespace but
            // whitespace counter is increased.
            // If the whitespace counter is increased to 4, replace it with a tab instead.
            case TOK_WHITESPACE:
                if (last.val() == TOK_WHITESPACE) {
                    int wc = last.count();

                    if (wc > 3) {
                        last = Token(TOK_TAB);

                        #ifndef NDEBUG
                        std::cerr << "Invalid handling of whitespace: " << wc
                        << " unhandled whitespaces in a row at " 
                        << lexer->line() << ':' << lexer->col();
                        #endif
                    }

                    if (wc == 3)
                        last = Token(TOK_TAB);
                    else if (wc < 3)
                        last = Token(TOK_WHITESPACE, "", wc + 1);
                } else {
                    vec.push_back(next);
                    
                }
                break;


            case TOK_ADD:
                if (last.val() == TOK_ADD)
                    last = Token(TOK_INCREMENT);
                else {
                    vec.push_back(next);
                    
                }
                break;


            case TOK_SUB:
                if (last.val() == TOK_SUB)
                    last = Token(TOK_DECREMENT);
                else {
                    vec.push_back(next);
                    
                }
                break;


            case TOK_DIV:
                switch (last.val()) {
                case TOK_DIV:
                    last = Token(TOK_LINE_COMMENT);
                    break;
                case TOK_MUL:
                    last = Token(TOK_COMMENT_END);
                    break;
                default:
                    vec.push_back(next);
                    break;
                }
                break;


            case TOK_MUL:
                switch (last.val()) {
                case TOK_DIV:
                    last = Token(TOK_COMMENT_START);
                    break;
                case TOK_TYPE:
                    last = Token(TOK_PTR, last.data());
                    break;
                case TOK_PTR:
                    last = Token(TOK_PTR, last.data(), last.count() + 1);
                    break;
                default:
                    vec.push_back(next);
                    break;
                }
                break;


            case TOK_LEFT_ANGLE:
                if (last.val() == TOK_LEFT_ANGLE)
                    last = Token(TOK_LEFT_SHIFT);
                else {
                    vec.push_back(next);
                }
                break;
            

            case TOK_RIGHT_ANGLE:
                switch (last.val()) {
                case TOK_RIGHT_ANGLE:
                    last = Token(TOK_RIGHT_SHIFT);
                    break;
                case TOK_SUB:
                    last = Token(TOK_ARROW);
                    break;
                default:
                    vec.push_back(next);
                    break; 
                }
                break;
            
            
            case TOK_AMPERSAND:
                if (last.val() == TOK_AMPERSAND)
                    last = Token(TOK_AND);
                else {
                    vec.push_back(next);
                }
                break;


            case TOK_VERTICAL_BAR:
                if (last.val() == TOK_VERTICAL_BAR)
                    last = Token(TOK_OR);
                else {
                    vec.push_back(next);
                    
                }
                break;


            case TOK_ASSIGN:
                switch (last.val()) {
                case TOK_ADD:
                    last = Token(TOK_ADD_ASSIGN);
                    break;
                case TOK_SUB:
                    last = Token(TOK_SUB_ASSIGN);
                    break;
                case TOK_MUL:
                    last = Token(TOK_MUL_ASSIGN);
                    break;
                case TOK_DIV:
                    last = Token(TOK_DIV_ASSIGN);
                    break;
                case TOK_EXCLAMATION:
                    last = Token(TOK_NOT_EQUALS);
                    break;
                case TOK_AMPERSAND:
                    last = Token(TOK_AND_ASSIGN);
                    break;
                case TOK_VERTICAL_BAR:
                    last = Token(TOK_OR_ASSIGN);
                    break;
                case TOK_TILDE:
                    last = Token(TOK_TILDE_ASSIGN);
                    break;
                case TOK_LEFT_ANGLE:
                    last = Token(TOK_EQUALS_SMALLER);
                    break;
                case TOK_RIGHT_ANGLE:
                    last = Token(TOK_EQUALS_LARGER);
                    break;
                case TOK_ASSIGN:
                    last = Token(TOK_EQUALS);
                    break;
                case TOK_LEFT_SHIFT:
                    last = Token(TOK_LEFT_SHIFT_ASSIGN);
                    break;
                case TOK_RIGHT_SHIFT:
                    last = Token(TOK_RIGHT_SHIFT_ASSIGN);
                    break;
                default:
                    vec.push_back(next);
                    break;
                }
                break;


            default:
                vec.push_back(next);
                break;
            }

            if (next.val() == TOK_EOF)
                break;

        } else /* if Result of next_token() was an Exception */ {
            lexer->errors_.push_back(next_res.unwrap_err());
        }

        if (eof_reached)
            break;
    }

    if ((!vec.empty() && vec.back().val() != TOK_EOF) || vec.empty()) {
        salt::print_warning("could not read EOF; please try ending your file with a newline character");
        vec.push_back(Token(TOK_EOF));
    }

    if (salt::dboutv.is_active())
        for (const Token& tok : vec)
            salt::dboutv << tok << std::endl;

    return vec;
}

std::vector<Token> tokenize(const char* str) {
    Lexer* lexer = Lexer::get();
    return lexer->tokenize(str);
}