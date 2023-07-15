#ifndef SALT_MINIREGEX_H
#define SALT_MINIREGEX_H
#include <vector>
#include <string>
extern const char ALLOWED_CHARS[];
extern std::vector<std::string> TYPES;
extern const char* DEFAULT_TYPES[];
extern const int ALLOWED_CHARS_LEN;
extern const int ALLOWED_SYMBOLS_START;

namespace MiniRegex
{
void fill_types();
}
inline bool is_digit(const char ch);
inline bool is_whitespace(const char ch);
bool is_type(const char* s);
bool is_type(const std::string& s);
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