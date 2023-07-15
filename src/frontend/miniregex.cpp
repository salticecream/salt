#include "miniregex.h"
// Data
const char ALLOWED_CHARS[] = 
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"@#%&/()=[]<>+-*,.;:| \n\t\r";
std::vector<std::string> TYPES;
const char* DEFAULT_TYPES[] = {"int", "void"};
const int ALLOWED_CHARS_LEN = 88; // need manual edit
const int ALLOWED_SYMBOLS_START = 62;

// Functions

// Add the default list of types to TYPES.
void MiniRegex::fill_types() {
    if (TYPES.empty())
        for (const char* type : DEFAULT_TYPES)
            TYPES.push_back(type);
}

bool is_alphabetic (const char ch) {
    if (ch >= 'a' && ch <= 'z')
        return true;
    else if (ch >= 'A' && ch <= 'Z')
        return true;
    /* else */
        return false;
}


bool is_alphanumeric(const char ch) {
    if (is_digit(ch))
        return true;
    else if (is_alphabetic(ch))
        return true;
    /* else */
        return false;
}


bool is_valid_symbol(const char ch) {
    for (int i = ALLOWED_SYMBOLS_START; ALLOWED_CHARS[i] != '\0'; i++)
        if (ch == ALLOWED_CHARS[i])
            return true;
    return false;
}


bool is_allowed(const char ch) {
    if (is_digit(ch))
        return true;
    else if (is_alphabetic(ch))
        return true;
    else if (is_valid_symbol(ch))
        return true;
    /* else */
        return false;
}

bool is_integer(const char* s) {
    if (*s == '-')
        s++;
        
    while (*s) {
        if (!is_digit(*s))
            return false;
        s++;
    }
    return true;
}

bool is_type(const char* s) {
    for (auto& str : TYPES)
        if (str == s)
            return true;
    return false;
}

bool is_type(const std::string& s) {
    for (auto& str : TYPES)
        if (str == s)
            return true;
    return false;
}