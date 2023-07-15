#ifndef SALT_MINIREGEX_H
#define SALT_MINIREGEX_H
const char ALLOWED_CHARS[] = 
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"@#%&/()=[]<>+-*,.;:| \n\t\r";
const int ALLOWED_CHARS_LEN = 88; // need manual edit
const int ALLOWED_SYMBOLS_START = 62;

inline bool is_digit(const char ch);
inline bool is_whitespace(const char ch);
bool is_alphanumeric(const char ch);
bool is_alphabetic(const char ch);
bool is_valid_symbol(const char ch);
bool is_allowed(const char ch);
bool is_integer(const char* s);

inline bool is_digit(const char ch) {
    return (ch >= '0' && ch <= '9');
}

inline bool is_whitespace(const char ch) {
    return (ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ');
}

#endif