#pragma once

#include <vector>
#include <string>
#include "types.h"
#include "../common.h"

extern const char ALLOWED_CHARS[];
extern std::vector<Type> TYPES;
extern const int ALLOWED_CHARS_LEN;
extern const int ALLOWED_SYMBOLS_START;
extern const char SEPARATOR_CHARS[];

namespace MiniRegex
{
void fill_types();
}

inline bool is_digit(const char ch);
inline bool is_whitespace(const char ch);
inline bool ends_with_whitespace(const std::string& s);
inline bool is_whitespace_or_eof(const char ch);
inline bool is_separator(const char ch);
inline bool is_valid_identifier(const char* str);
inline std::string trim_last(const std::string& s);
std::string trim_whitespace(const std::string& s);
bool is_type(const char* s);
bool is_type(const std::string& s);
bool is_alphanumeric(const char ch);
bool is_alphanumeric(const char* str);
bool is_alphabetic(const char ch);
bool is_valid_symbol(const char ch);
bool is_allowed(const char ch);
bool is_integer(const char* str);
bool is_pointer(const std::string& s);
bool string_ends_with(const char* str, const char* end);
bool string_starts_with(const char* str, const char* end);
char last_whitespace(std::string& s);

inline bool is_digit(const char ch) {
    return (ch >= '0' && ch <= '9');
}

inline bool is_whitespace(const char ch) {
    return (ch == '\n' || ch == '\r' || ch == '\t' || ch == ' ');
}

inline bool is_whitespace_or_eof(const char ch) {
    return (is_whitespace(ch) || ch == EOF);
}

inline bool ends_with_whitespace(const std::string& s) {
    return is_whitespace(s[s.size() - 1]);
}

inline bool ends_with_whitespace_or_eof(const std::string& s) {
    return is_whitespace_or_eof(s[s.size() - 1]);
}

inline bool is_separator(const char ch) {
    return (is_valid_symbol(ch) || ch == EOF) && ch != '_';
}

inline std::string trim_last(const std::string& s) {
    return std::string(s.substr(0, s.size() - 1));
}

inline bool is_valid_identifier(const char* str) {
    char start = str[0];
    if (!is_alphabetic(start) && start != '_')
        return false;

    while (*++str != '\0') {
        if (!is_alphanumeric(*str) && *str != '_')
            return false;
    }

    return true;
}