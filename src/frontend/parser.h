#pragma once

#include "ast.h"
#include "tokens.h"
#include "../common.h"
#include "irgenerator.h"
#include <vector>

// typedef void ParserReturnType (temporaily for parse() function)
typedef void ParserReturnType;


class Parser {
private:
    int current_idx;
    static Parser* instance; // Singleton just like Lexer.
    const std::vector<Token>& vec;;
    Parser(const std::vector<Token>& vec_ref);

    // Helper functions for Parser::parse().
    salt::Result<Expression> parse_number_expr();
    salt::Result<Expression> parse_paren_expr();
    salt::Result<Expression> parse_ident_expr();
    salt::Result<Expression> parse_primary();
    salt::Result<Expression> parse_expression();
    salt::Result<Expression> parse_binop_rhs(int prec, Expression lhs);
    salt::Result<Expression> parse_if_expr();
    salt::Result<Expression> parse_repeat_expr();
    salt::Result<std::unique_ptr<DeclarationAST>> parse_declaration();
    salt::Result<std::unique_ptr<FunctionAST>> parse_function();
    salt::Result<std::unique_ptr<DeclarationAST>> parse_extern();
    salt::Result<std::unique_ptr<FunctionAST>> parse_top_level_expr();

    

    void skip_whitespace();
    void next();
    bool can_go_next();

    salt::Result<void> handle_extern();
    salt::Result<void> handle_function();
    salt::Result<void> handle_top_level_expr();
    salt::Result<void> handle_if_expr();

//    const Token& current();
//    const Token& current_no_whitespace();


public:
    static Parser* get();
    static Parser* get(const std::vector<Token>& vec_ref);
    static void destroy();
    ParserReturnType parse();
};

class ParserException : public salt::Exception {
private:
    std::string what_;
public:
    ParserException(const Token& tok, const char* str);
    ParserException(const char* str);
};
