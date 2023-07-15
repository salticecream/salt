/* disclaimer:
this file is part of my salt project, which uses the gnu gplv3 license
*/
#ifndef SALT_TOKENS_H
#define SALT_TOKENS_H
#include <string>
#include <iostream>

enum Token_e {
    // end of file
    TOK_EOF         = 0,

    // commands
    TOK_FN          = 1,
    TOK_EXTERN      = 2,
    TOK_STRUCT      = 3,

    // (will not be implemented atm) keywords

    // other
    TOK_IDENT       = 4,
    TOK_NUMBER      = 5,    // only int
    TOK_PTR         = 6,    // only void*
    TOK_LET         = 7,
    TOK_TYPE        = 8,
    TOK_EOL         = 9,    // end of line
    TOK_EOS         = 10,   // end of statement
    TOK_TOTAL
    


    // (will not be implemented atm) types
    
}; 

class Token {
private:

    int val;
    std::string _data;


public:

    Token(int value);
    Token(int value, std::string data);

    explicit operator int() const;
    operator bool() const;

    // Converts a token to a string.
    std::string str() const;

    // Retrieves the data from a string, if there is any.
    std::string& data() const;

    bool has_data() const;

    enum DataType {
        VOID = 0,
        INT,
        STRING,
    };

    // Make the Token printable with std::cout and std::cerr
    friend std::ostream& operator<<(std::ostream& os, const Token& token);

};



#endif