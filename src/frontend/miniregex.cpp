#include "miniregex.h"
#include "types.h"

// Data
const char ALLOWED_CHARS[] = 
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!\"@#%&/()=[]<>+-*,.;:|_ \n\t\r'";
const int ALLOWED_CHARS_LEN = sizeof(ALLOWED_CHARS) - 1;
const int ALLOWED_SYMBOLS_START = 62;


// Functions
bool is_alphabetic (const char ch) {
    if (ch >= 'a' && ch <= 'z')
        return true;
    else if (ch >= 'A' && ch <= 'Z')
        return true;
    
    return false;
}


bool is_alphanumeric(const char ch) {
    if (is_digit(ch))
        return true;
    else if (is_alphabetic(ch))
        return true;
    
    return false;
}

bool is_alphanumeric(const char* str) {
    while (*str) {
        if (!is_alphanumeric(*str))
            return false;
        str++;
    }
    return true;
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
    
    return false;
}

bool is_integer(const char* s) {
    if (*s == '-')
        s++;

    bool starts_with_zero = false;

    // handle octal, binary, hexadecimal integers too
    if (*s == '0') {
        s++;
        starts_with_zero = true;
    }

    if (*s == 'b' || *s == 'B' || *s == 'x' || *s == 'X') {
        if (!starts_with_zero)
            return false;
        s++;
    }
        
    while (*s) {
        if (!is_digit(*s))
            return false;
        s++;
    }
    return true;
}

bool is_type(const char* s) {

    return salt::all_types.count(s);
}

bool is_type(const std::string& s) {
    return salt::all_types.count(s);
}

bool is_pointer(const std::string& s) {
    const size_t& length = s.size();
    if (length < 2)
        return false;

    // check if the string is of kind "type*"
    if (s[length - 1] == '*') {
        return is_type(s.substr(0, length - 2));
    } else {
        return false;
    }
}

bool string_ends_with(const char* str, const char* end) {
    int str_length = strlen(str);
    int end_length = strlen(end);
    if (str_length < end_length)
        return false;

    const char* str_substr = str + str_length - end_length;
    return (strcmp(str_substr, end) == 0);
}

bool string_starts_with(const char* str, const char* start) {
    int i;
    for (i = 0; start[i] != '\0' && str[i] != '\0'; i++) {
        if (str[i] != start[i])
            return false;
    }

    // if str does not fully contain start, then return false
    if (str[i] == '\0')
        return start[i] == '\0';

    return true;
}

// Returns the last whitespace in a string, or a null byte if the string
// does not end with one.
char last_whitespace(const std::string& s) {
    const char last_char = s[s.size() - 1];
    if (is_whitespace(last_char))
        return last_char;
    else
        return '\0';
}

// Removes trailing whitespace from a string.
[[nodiscard]]
std::string trim_whitespace(std::string& str) {
    int whitespaces = 0;
    for (int i = str.size() - 1; i >= 0; i--)
        if (is_whitespace(str[i]))
            whitespaces++;
        else
            break;
    return str.substr(0, str.size() - whitespaces);
}