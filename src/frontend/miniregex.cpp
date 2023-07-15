#include "miniregex.h"

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