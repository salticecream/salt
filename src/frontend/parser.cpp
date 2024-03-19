#include "parser.h"
#include "operators.h"
#include "ast.h"
#include "miniregex.h"
#define PARSER_MAX_ERRORS 20

using namespace salt;

Parser* Parser::instance = nullptr;

Parser* Parser::get() {
    return Parser::instance;
}

Parser* Parser::get(const std::vector<Token>& vec) {
    Parser::destroy();
    Parser::instance = new Parser(vec);
    return Parser::instance;
}

void Parser::destroy() {
    if (Parser* p = Parser::get())
        delete p;
    Parser::instance = nullptr;
}

Parser::Parser(const std::vector<Token>& vec_ref) : vec(vec_ref) {
    this->current_idx = 0;
}

// Helper functions for Parser::parse().
void Parser::skip_whitespace() {
    while (1) {
        Token_e val = vec[current_idx].val();
        if (val == TOK_WHITESPACE || val == TOK_TAB || val == TOK_EOL)
            current_idx++;
        else
            break;
    }
}

bool Parser::can_go_next() {
    return current_idx < vec.size() - 1;
}

void Parser::next() {
    if (current_idx == vec.size() - 1)
        throw std::exception("current_idx reached vec size");

    else if (vec[current_idx].val() == TOK_EOF)
        throw std::exception("trying to access token vector past EOF");

    else if (current_idx > vec.size() - 1)
        throw std::exception("current_idx exceeded vec size");

    current_idx++;
    skip_whitespace();
}

Result<Expression> Parser::parse_number_expr() {
    bool negative = false;

    if (vec[current_idx].val() == TOK_SUB) {
        negative = true;
        if (vec[current_idx + 1].val() != TOK_NUMBER)
            return ParserException(vec[current_idx], "expected number after unary \"-\" (negating expressions NYI)");
        this->next();
    }

    int val = negative ? -std::atoi(vec[current_idx].data().c_str()) : std::atoi(vec[current_idx].data().c_str());
        
    auto res = std::make_unique<IntExprAST>(vec[current_idx], val);
    this->next();
    return std::move(res);
}

// Note: NOT for function calls. This is for expressions such as 3 * (5 + 3), NOT func(1, 2).
Result<Expression> Parser::parse_paren_expr() {
    this->next(); // we must move forward from this left paren
    Result<Expression> res = parse_expression();
    if (vec[current_idx].val() != TOK_RIGHT_BRACKET)
        // We expected parenthesis, we didnt get right parenthesis
        return ParserException(vec[current_idx], "expected \")\"");
    this->next(); // ok, move forward from this right paren as well
    return res.unwrap();
}

/// @todo: add type here, dont assume "int" (but how?)
Result<Expression> Parser::parse_ident_expr() {
    std::string ident_name = vec[current_idx].data();
    if (!is_valid_identifier(ident_name.c_str()))
        return ParserException(vec[current_idx], "identifiers must start with a letter");

    int ident_idx = current_idx;
    
    this->next();

    // Check if this is NOT a function call, if so, return the identifier as its own expression
    if (vec[current_idx].val() != TOK_LEFT_BRACKET)
        return std::make_unique<VariableExprAST>(vec[ident_idx]);
    

    // this identifier is a function call. treat it like one.
    std::vector<Expression> args;

    // skip (
    this->next();
    
    
    if (vec[current_idx].val() != TOK_RIGHT_BRACKET)
        // uh oh, this function call has arguments! handle accordingly
        while (1) {
            // current argument
            // const Token& tok = vec[current_idx];
            // const std::string& arg_name = tok.data();

            /// @todo: add more types!!!
            Result<Expression> arg_res = parse_expression();
            if (!arg_res)
                return arg_res.unwrap_err();

            Expression arg = arg_res.unwrap();

            args.push_back(std::move(arg));
            
            // we should be at a comma, or a ')' now. because of parse_expression()
            // which already calls next() for us
            if (vec[current_idx].val() == TOK_COMMA)
                this->next();
            else if (vec[current_idx].val() == TOK_RIGHT_BRACKET)
                break;
            else
                return ParserException(vec[ident_idx], "expected ',' or ')");
        }

    // we finally reached the end of the function call, current token is ). skip that.
    this->next();

    return std::make_unique<CallExprAST>(vec[ident_idx], std::move(args));
}

Result<Expression> Parser::parse_primary() {
    Token_e val = vec[current_idx].val();
    switch (val) {
    // here we don't need to use this->next() because
    // that is done in the following functions.
    case TOK_IDENT:
        return parse_ident_expr();
    case TOK_NUMBER:
    case TOK_SUB:
        return parse_number_expr();
    case TOK_LEFT_BRACKET:
        return parse_paren_expr();
    case TOK_IF:
        return parse_if_expr();
    case TOK_REPEAT:
        return parse_repeat_expr();
    default:
        return ParserException(vec[current_idx],
            "expected primary expression (that is, a number literal, a function call, an identifier, \"if\" or \"repeat\" keywords, or \"(\")");
    }
}

Result<Expression> Parser::parse_expression() {
    if (Result<Expression> lhs_res = parse_primary())
        return parse_binop_rhs(0, std::move(lhs_res.unwrap()));
    else
        return lhs_res.unwrap_err();
}

Result<Expression> Parser::parse_binop_rhs(int prec, Expression lhs) {
    while (true) {
        // We are currently at what we presume to be a binary op
        // Therefore, get its precedence.
        const Token& op = vec[current_idx]; 
        int tok_prec = BinaryOperator::get_precedence(op.val());

        // We passed in 0 from the start. If 0 is higher than tok_prec
        // it means this token has -1 precedence, it's not a binop. So just return lhs
        // More generally, if "op" has lower precedence than what we passed in to the function
        // then our calculation (of the LHS of op) is done.
        if (tok_prec < prec)
            return lhs;

        // Ok, op is a binary operator and it binds more tightly than whatever we passed in.
        // Directly following op we expect a primary expression. Therefore, jump to it.
        this->next(); 

        // We evaluate the (primary) expression on the rhs of op. For example, if the expression is 1 + 2 * 3
        // then lhs is 1, op is '+' and rhs is (currently) 2.
        Result<Expression> rhs_res = parse_primary(); 

        // If parsing the primary expression fails, we return the error contained within.
        if (!rhs_res)
            return rhs_res.unwrap_err();

        Expression rhs = rhs_res.unwrap();
    
        // parse_primary() calls this->next(). So we are currently at what we assume to be another binop

        // Now we check to see if the next token binds more tightly than op. If so, we need to calculate that first.
        // In our 1 + 2 * 3 example, the RHS of op (2), should be calculated using the *, not with the +.
        Token_e next_token = vec[current_idx].val();
        int next_prec = BinaryOperator::get_precedence(next_token);

        // If next_prec binds more tightly (which it does in this case), then we need to calculate rhs first.
        if (tok_prec < next_prec) {
            Result<Expression> next_rhs_res = parse_binop_rhs(tok_prec, std::move(rhs));
            // If parsing this new rhs fails, then we return the error contained within.
            if (!next_rhs_res)
                return next_rhs_res.unwrap_err();


            // This calculation will recursively occur until a "weaker" token is found.
            rhs = next_rhs_res.unwrap();
        }
    
        // And after we have found what our rhs must finally be, we create the binary expression.

        lhs = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));

        // And the loop repeats.
    }
}

Result<Expression> Parser::parse_if_expr() {
    // Assume we're at the keyword "if"
    if (vec[current_idx].val() != TOK_IF)
        return ParserException(vec[current_idx], "expected keyword \"if\"");
    this->next();

    // Read the condition
    Result cond_res = parse_expression();
    if (!cond_res)
        return cond_res.unwrap_err();

    // Expect a "then"
    if (vec[current_idx].val() != TOK_THEN)
        return ParserException(vec[current_idx], "expected keyword \"then\" after condition");
    this->next();

    // Read the following expr
    Result true_expr_res = parse_expression();
    if (!true_expr_res)
        return true_expr_res.unwrap_err();

    // Read an "else" (all "ifs" must always evaluate to something right now)
    // In the future, if without else will be possible
    if (vec[current_idx].val() != TOK_ELSE)
        return ParserException(vec[current_idx], "expected keyword \"else\" after expression");
    this->next();

    // Read the following expr
    Result false_expr_res = parse_expression();
    if (!false_expr_res)
        return false_expr_res.unwrap_err();

    /// @todo: After adding types, check that both true_expr and false_expr both have the same type

    // We've reached the end with no errors
    // Return a new if expression with cond, true_expr and false_expr.

    return std::make_unique<IfExprAST>(cond_res.unwrap(), true_expr_res.unwrap(), false_expr_res.unwrap());

}


Result<std::unique_ptr<DeclarationAST>> Parser::parse_declaration() {
    // We assume that the current token is TOK_FN
    if (vec[current_idx].val() != TOK_FN)
        return ParserException(vec[current_idx], "expected keyword \"fn\"");
    this->next();

    // We could also take the index of the function and use that in std::make_unique at the end
    // but since we are using the token for other things we just make a reference to it
    const Token& function_identifier_token = vec[current_idx];
    const std::string& function_name = function_identifier_token.data();
    if (!is_valid_identifier(function_name.c_str()))
        return ParserException(vec[current_idx], "identifiers must start with a letter");
    
    this->next();
    if (vec[current_idx].val() != TOK_LEFT_BRACKET)
        return ParserException(vec[current_idx], "expected \"(\" as part of function declaration");

    

    // create an arg vector and populate it, just like we did with the function call
    // declaration in parse_ident_expr()
    std::vector<Variable> args;

    this->next(); // now we are either at the first argument or at a ')'
    if (vec[current_idx].val() != TOK_RIGHT_BRACKET)
        while (1) {            
            // current argument

            const Token& tok = vec[current_idx];
            if (tok.val() != TOK_IDENT) /// @todo: make this TOK_TYPE (for typed variable)
                return ParserException(vec[current_idx], "expected identifier");
            const std::string& arg_name = tok.data();

            /// @todo: add more types!!!
            Variable arg = Variable(arg_name, DEFAULT_TYPES[DT_INT]);

            args.push_back(std::move(arg));
            this->next(); // we should be at a comma, or a ')' now
            if (vec[current_idx].val() == TOK_COMMA)
                this->next();
            else if (vec[current_idx].val() == TOK_RIGHT_BRACKET)
                break;
            else
                return ParserException(vec[current_idx], "expected ',' or ')'");
        }
    // we are currently at the ')' symbol, now that we are done just jump over it
    if (vec[current_idx].val() != TOK_RIGHT_BRACKET)
        return ParserException(vec[current_idx], "expected \")\"");
    this->next();

    return std::make_unique<DeclarationAST>(function_identifier_token, std::move(args));
}

Result<std::unique_ptr<FunctionAST>> Parser::parse_function() {
    // assume that the current token is TOK_FN
    auto decl_res = parse_declaration(); // consumes that token
    if (!decl_res)
        return decl_res.unwrap_err();

    std::unique_ptr<DeclarationAST> decl = decl_res.unwrap();

    if (vec[current_idx].val() != TOK_COLON)
        return ParserException(vec[current_idx], "expected \":\" after non-extern function declaration");
    
    this->next(); // move fd from ':'
    Result<Expression> body_res = parse_expression();
    
    // If parsing an expression for the body fails, then return the error contained inside
    if (!body_res)
        return body_res.unwrap_err();

    Expression body = body_res.unwrap();

    return std::make_unique<FunctionAST>(std::move(decl), std::move(body));
}

Result<std::unique_ptr<DeclarationAST>> Parser::parse_extern() {
    // assume the current token is TOK_EXTERN
    this->next();
    if (vec[current_idx].val() != TOK_FN)
        return ParserException(vec[current_idx], "expected keyword \"fn\" after \"extern\" keyword");
    
    // no this->next() here, bcs parse_declaration() consumes that token
    return parse_declaration();
}

// only for the inteactive command line.
// must be removed later when we start compiling programs.
// Works by creating an anonymous function without params
// that capture the top level expression.
Result<std::unique_ptr<FunctionAST>> Parser::parse_top_level_expr() {
    Result<Expression> expr_res = parse_expression();
    if (!expr_res)
        return expr_res.unwrap_err();

    Expression expr = expr_res.unwrap();
    auto decl = std::make_unique<DeclarationAST>(Token(TOK_NONE), std::vector<Variable>());
    return std::make_unique<FunctionAST>(std::move(decl), std::move(expr));
}

Result<Expression> Parser::parse_repeat_expr() {
    if (vec[current_idx].val() != TOK_REPEAT)
        return ParserException(vec[current_idx], "expected repeat keyword");
    int rep_line = vec[current_idx].line();
    int rep_col = vec[current_idx].col();
    this->next();


    int index_before_parse = current_idx;
    Expression success_ret_value = nullptr;
    ParserException fail_ret_value = ParserException(vec[index_before_parse], "Uninitialized parser exception");

    Result expr_res = parse_expression();
    if (expr_res)
        success_ret_value = expr_res.unwrap();
    else
        /*fail_ret_value = */ throw ParserException(vec[index_before_parse], expr_res.unwrap_err().what());


    // Try to skip the following colon.
    // If the previous parsing failed, then simply return that error.
    // If there is no colon following the expression, but parsing did not fail,
    // that's still an error. Return that instead.
    // If there is a colon after the expression, keep going and try to grab the following expression.

    if (vec[current_idx].val() == TOK_COLON) {
        this->next();
        if (expr_res) {
            if (Result repeat_body_res = parse_expression()) {
                Expression repeat_body = repeat_body_res.unwrap();
                return std::make_unique<RepeatAST>(
                    rep_line, rep_col, std::move(success_ret_value), std::move(repeat_body)
                );
            }
            else
                return repeat_body_res.unwrap_err();

        }
        else
            return fail_ret_value;
    } else {
        if (expr_res)
            return ParserException(vec[index_before_parse], "expected : after repeat expression");
        else
            return fail_ret_value;
    }
}

Result<void> Parser::handle_extern() {
    if (Result<std::unique_ptr<DeclarationAST>> decl_res = parse_extern()) {
        std::unique_ptr<DeclarationAST> decl = decl_res.unwrap();
        llvm::Function* generated_ir = decl->code_gen();
        salt::dbout << "Successfully parsed declaration ";
        if (SALT_DEBUG_PRINT)
            generated_ir->print(llvm::errs());
        salt::dbout << " at: "
            << decl->line()
            << ':'
            << decl->col()
            << std::endl;
        return Result_e::OK;
    }
    else {
        if (can_go_next())
            this->next();
        return decl_res.unwrap_err();
    }
}

Result<void> Parser::handle_top_level_expr() {
    if (Result<std::unique_ptr<FunctionAST>> fn_res = parse_top_level_expr()) {
        std::unique_ptr<FunctionAST> func = fn_res.unwrap();
        llvm::Function* generated_ir = func->code_gen();
        salt::dbout << "Successfully parsed top level expression ";
        if (SALT_DEBUG_PRINT)
            generated_ir->print(llvm::errs());
        salt::dbout << " at: "
            << func->decl()->line()
            << ':'
            << func->decl()->col()
            << std::endl;
        return Result_e::OK;
    }
    else {
        if (can_go_next())
            this->next();
        return fn_res.unwrap_err();
    }
}

Result<void> Parser::handle_function() {
    if (Result<std::unique_ptr<FunctionAST>> fn_res = parse_function()) {
        std::unique_ptr<FunctionAST> func = fn_res.unwrap();
        llvm::Function* generated_ir = func->code_gen();
        salt::dbout << "Successfully parsed function ";
        if (SALT_DEBUG_PRINT)
            generated_ir->print(llvm::errs());
        salt::dbout << " at: "
            << func->decl()->line()
            << ':'
            << func->decl()->col()
            << std::endl;
        return Result_e::OK;
    }
    else {
        if (can_go_next())
            this->next();
        return fn_res.unwrap_err();
    }


}

Result<void> a;

// Don't use fucking unwraps here, only for testing purposes
ParserReturnType Parser::parse() {
    // for (int i = 0; i < 1; i++)
    try {
        while (1) {
            Result<void> res;
            if (current_idx >= vec.size())
                return;
            Token_e val = vec[current_idx].val();

            switch (val) {
            case TOK_EOF:
                return;
            case TOK_EOL:
            case TOK_WHITESPACE:
            case TOK_TAB:
                this->next();
                break;
            case TOK_EXTERN:
                res = handle_extern();
                if (!res)
                    std::cout << res.unwrap_err().what() << std::endl;
                break;
            case TOK_FN:
                res = handle_function();
                if (!res)
                    std::cout << res.unwrap_err().what() << std::endl;
                break;
            case TOK_NUMBER:
            case TOK_SUB:
            case TOK_REPEAT:
            case TOK_IF:
                res = handle_top_level_expr();
                if (!res)
                    std::cout << res.unwrap_err().what() << std::endl;
                break;
            default: 
                res = handle_function();
                if (!res)
                    std::cout << res.unwrap_err().what() << std::endl;
                break;
            }
        }
    }
    catch (const ParserException& e) {
        std::cerr << e.what() << std::endl;
    }

    catch (const salt::Exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}


ParserException::ParserException(const Token& tok, const char* str) :
    Exception((std::to_string(tok.line())
        + ':'
        + std::to_string(tok.col())
        + ": found token "
        + tok.str()
        + ", "
        + str
        )) {}



