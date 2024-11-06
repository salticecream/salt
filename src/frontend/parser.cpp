#include "parser.h"
#include "operators.h"
#include "ast.h"
#include "miniregex.h"
#include "irgenerator.h"

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

Parser::Parser(const std::vector<Token>& vec_ref) : vec(vec_ref), is_parsing_extern(false) {
    this->current_idx = 0;
    this->current_scope = 0;
    this->line_just_started = true;
}

// Helper functions for Parser::parse().
bool Parser::skip_whitespace() {
    int spaces_in_a_row = 0;
    bool looping = true;
    bool return_val = false;

    while (looping) {
        Token_e val = vec[current_idx].val();
        switch (val) {
        case TOK_WHITESPACE:
            current_idx++;
            spaces_in_a_row++;
            ASSERT(spaces_in_a_row < 4);
            break;
        case TOK_TAB:
            current_idx++;
            spaces_in_a_row = 0;
            current_scope += line_just_started;
            break;
        case TOK_EOL:
            current_idx++;
            line_just_started = true;
            current_scope = 0;
            spaces_in_a_row = 0;
            return_val = true;
            break;
        default:
            looping = false;
            spaces_in_a_row = 0;
            line_just_started = false;
        }
    }

    return return_val;
}

// warning, this function does not set current_scope correctly
// if you use it remember to restore current_scope!
bool Parser::skip_whitespace_back() {
    bool looping = true;
    bool return_val = false;

    while (looping && current_idx > 0) {
        Token_e val = vec[current_idx].val();
        switch (val) {
        case TOK_WHITESPACE:
            current_idx--;
            break;
        case TOK_TAB:
            current_idx--;
            break;
        case TOK_EOL:
            current_idx--;
            return_val = true;
            break;
        default:
            looping = false;
        }
    }

    return return_val;
}

// warning, this function does not set current_scope correctly
// if you use it remember to restore current_scope!
ParserNextType Parser::back() {
    int old = current_idx;

    if (current_idx < 0)
        print_fatal("Parser's current index is negative");

    if (current_idx == 0)
        print_fatal("Trying to call Parser::back() when parser's index is 0");

    current_idx--;
    bool skipped_newline = skip_whitespace_back();
    return { old - current_idx, skipped_newline };
    
}

bool Parser::can_go_next() {
    return current_idx < vec.size() - 1;
}

ParserNextType Parser::next() {
    int cur_pos = current_idx;
    if (current_idx == vec.size() - 1)
        salt::print_fatal("current_idx reached vec size");

    else if (vec[current_idx].val() == TOK_EOF)
        salt::print_fatal("trying to access token vector past EOF");

    else if (current_idx > vec.size() - 1)
        salt::print_fatal("current_idx EXCEEDED vec size");

    current_idx++;
    bool new_line_skipped = skip_whitespace();
    return { current_idx - cur_pos, new_line_skipped };
}

const Token& Parser::current() const { return vec[current_idx]; }

Result<Expression> Parser::parse_number_expr() {
    bool negative = false;

    if (vec[current_idx].val() == TOK_SUB) {
        negative = true;
        if (vec[current_idx + 1].val() != TOK_NUMBER)
            return ParserException(vec[current_idx], "expected number after unary \"-\" (negating expressions NYI, please multiply by -1)");
        this->next();
    }

    int64_t val_int = 0;
    double val_float = (1.0e300 * 1.0e300) - (1.0e300 * 1.0e300); // NaN
    bool should_return_float = false;
    switch (vec[current_idx].count()) {
    case 0:
        break;
    case 1:
        should_return_float = true;
        break;
    default:
        print_fatal(f_string("Bad count for number token: %d (expected 0 or 1)", vec[current_idx].count()));
    }

    bool error = false;

    if (vec[current_idx].data() == TOKEN_STR_INF) {
        should_return_float = true;
        val_float = 1.0e300 * 1.0e300 * (1 - 2 * negative);
    } else {
        std::string string_to_parse;
        if (negative)
            string_to_parse += '-';
        string_to_parse += vec[current_idx].data();

        ParsedNumber pn = parse_num_literal(string_to_parse);
        if (pn.type < 0)
            error = true;

        switch (pn.type) {
        case PARSED_NEG_INT:
        case PARSED_POS_INT:
            val_int = static_cast<int64_t>(pn.u64);
            // print_warning(f_string("parsed integer: %lld", val_int));
            break;
        case PARSED_FLOAT:
            val_float = pn.f64;
            // print_warning(f_string("parsed floating value: %lf", val_float));
            break;
        case PARSED_BAD_RADIX:
        case PARSED_BAD_NUMBER:
        case PARSED_OVERFLOW:
        case PARSED_ERROR:
        default:
            break;
        }

    }
    
    int token_delta = this->next().delta;
    if (!error) {
        if (should_return_float)
            return std::make_unique<ValExprAST>(vec[current_idx - 1], val_float, SALT_TYPE_DOUBLE);
        else
            return std::make_unique<ValExprAST>(vec[current_idx - 1], val_int, SALT_TYPE_LONG);
    }

    print_error(f_string("%d:%d: invalid numeric literal",
        vec[current_idx - token_delta].line(),
        vec[current_idx - token_delta].col()));

    if (should_return_float)
        return std::make_unique<ValExprAST>(vec[current_idx - 1], val_float, SALT_TYPE_DOUBLE);
    else
        return std::make_unique<ValExprAST>(vec[current_idx - 1], val_int, SALT_TYPE_LONG);
}

Result<Expression> Parser::parse_string_expr() {
    ASSERT(vec[current_idx].val() == TOK_STRING);
    auto res = std::make_unique<ValExprAST>(vec[current_idx], vec[current_idx].data());
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
    if (res)
        return res.unwrap();
    else
        return res.unwrap_err();
}

/// @todo: add type here, dont assume "int" (but how?)
// ans: add an UnknownType, and convert it implicitly to the correct type when generating the code
Result<Expression> Parser::parse_ident_expr() {
    std::string ident_name = vec[current_idx].data();
    if (!is_valid_identifier(ident_name.c_str()))
        return ParserException(vec[current_idx], "identifiers must start with a letter");

    int ident_idx = current_idx;
    
    this->next();

    

    // Check if this is NOT a function call, if so, return the identifier as its own expression
    if (vec[current_idx].val() != TOK_LEFT_BRACKET) {

        // Get the type of that identifier
        TypeInstance ti = named_values[ident_name];
        if (!ti)
            return Exception(f_string("variable %s does not exist", ident_name.c_str()));

        salt::dboutv << f_string("Making variable with name %s and type `%s`\n", vec[ident_idx].data(), named_values[ident_name].str());
        return std::make_unique<VariableExprAST>(vec[ident_idx], ti);
    }
    

    // this identifier is a function call. treat it like one.
    std::vector<Expression> args;
    TypeInstance call_return_type = named_functions[ident_name];
    if (!call_return_type)
        return Exception(f_string("Function %s does not exist", ident_name.c_str()));

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

    return std::make_unique<CallExprAST>(vec[ident_idx], std::move(args), call_return_type);
}

Result<Expression> Parser::parse_reserved_constant() {
    Expression res = nullptr;
    const Token& cur = vec[current_idx];
    switch (cur.val()) {
    case TOK_NULL:
        res = std::make_unique<ValExprAST>(cur, int64_t(0), TypeInstance(SALT_TYPE_VOID, 1));
        break;
    case TOK_INF:
        res = std::make_unique<ValExprAST>(cur, 1.0e300 * 1.0e300, SALT_TYPE_DOUBLE);
        break;
    case TOK_NAN:
        res = std::make_unique<ValExprAST>(cur, (1.0e300 * 1.0e300) - (1.0e300 * 1.0e300), SALT_TYPE_DOUBLE);
        break;
    case TOK_TRUE:
        res = std::make_unique<ValExprAST>(cur, int64_t(1), SALT_TYPE_BOOL);
        break;
    case TOK_FALSE:
        res = std::make_unique<ValExprAST>(cur, int64_t(0), SALT_TYPE_BOOL);
        break;
    default:
        print_fatal(f_string("Expected reserved constant after call to Parser::parse_reserved_constant, found %s", vec[current_idx].str()));
    }
    this->next();
    return std::move(res);
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
    case TOK_STRING:
        return parse_string_expr();
    case TOK_LEFT_BRACKET:
        return parse_paren_expr();
    case TOK_IF:
        return parse_if_expr();
    case TOK_REPEAT:
        return parse_repeat_expr();
    case TOK_NULL:
    case TOK_INF:
    case TOK_NAN:
    case TOK_TRUE:
    case TOK_FALSE:
        return parse_reserved_constant();
    case TOK_MUL:
        return parse_deref();
    case TOK_RETURN:
        return parse_return();
    default:
        return ParserException(vec[current_idx],
            "expected primary expression (that is, a literal, a function call, an identifier, \"if\" or \"repeat\" keywords, or \"(\")");
    }
}

Result<Expression> Parser::parse_expression() {
    // if (Result<Expression> ret_res = parse_return())
    //    return ret_res.unwrap();

    if (Result<Expression> lhs_res = parse_primary()) {
        // We parsed a primary expression, and we are currently at what might be a binary op
        // If the binary op is on the same line as the expression, then create a binary expr
        // Otherwise, if we had to go to a new line to reach this binary op, just return the lhs
        int old_pos = current_idx;
        ParserNextType pnt = this->back();
        current_idx = old_pos;
        if (pnt.new_statement) 
            return lhs_res.unwrap();

        return parse_binop_rhs(0, std::move(lhs_res.unwrap()));
    }
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

        // Note that a type (like "int") is allowed here, if "op" is TOK_AS.
        if (op.val() == TOK_AS) {
            if (vec[current_idx].val() == TOK_TYPE) {
                Expression ty = std::make_unique<TypeExprAST>(vec[current_idx]);
                this->next();
                Expression res = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(ty));
                return parse_binop_rhs(BinaryOperator::get_precedence(TOK_AS), std::move(res));
                // return std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(ty));
            }
            else
                return ParserException(vec[current_idx], "expected a type after \"as\" keyword");
        }

        Result<Expression> rhs_res = parse_primary(); 

        // If parsing the primary expression fails, we return the error contained within.
        if (!rhs_res)
            return rhs_res.unwrap_err();

        Expression rhs = rhs_res.unwrap();
    
        // parse_primary() calls this->next(). So we are currently at what we assume to be another binop.
        // But we must check if there was a newline between this new binop and the primary we just parsed. 
        // In that case, we are done.
        int old_idx = current_idx;
        bool there_was_newline = this->back().new_statement;
        current_idx = old_idx;

        if (there_was_newline)
            return std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));

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
    const Token& if_token = current();

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

    return std::make_unique<IfExprAST>(if_token, cond_res.unwrap(), true_expr_res.unwrap(), false_expr_res.unwrap());

}


Result<std::unique_ptr<DeclarationAST>> Parser::parse_declaration() {
    named_values.clear();
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
    std::vector<std::unique_ptr<VariableExprAST>> args;

    this->next(); // now we are either at the first argument or at a ')'
    if (vec[current_idx].val() != TOK_RIGHT_BRACKET)
        while (1) {            
            // current argument

            const Token& tok = vec[current_idx];
            if (tok.val() != TOK_TYPE)
                return ParserException(vec[current_idx], "expected type");
            const salt::Type* type = salt::all_types[tok.data()];
            int ptr_layers = tok.count();

            if (!type)
                type = SALT_TYPE_ERROR;
            this->next(); // go to the name of the argument


            const std::string& arg_name = vec[current_idx].data();
            salt::dbout << f_string("arg_name : %s\n", arg_name.c_str());

            /// @todo: add more types!!!
            
            std::unique_ptr<VariableExprAST> arg = nullptr;
            if (!ptr_layers) {
                arg = std::make_unique<VariableExprAST>(vec[current_idx], type);
                named_values.insert({ vec[current_idx].data(), type });
            } else {
                arg = std::make_unique<VariableExprAST>(vec[current_idx], TypeInstance(type, ptr_layers));
                named_values.insert({ vec[current_idx].data(), TypeInstance(type, ptr_layers) });
            }

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

    // If this function returns anything, we need to check the return value before creating the declaration
    // the function will show this by having a -> and then the return type
    // Otherwise, we assume void

    TypeInstance return_type = SALT_TYPE_VOID;

    if (vec[current_idx].val() == TOK_ARROW) {
        this->next();
        std::string& type_name = vec[current_idx].data();
        
        if (type_name.empty() || !is_type(type_name))
            return ParserException(vec[current_idx], "expected type name after -> symbol");
        
        else if (const Type* new_return_type = salt::all_types[type_name]) {
            if (int count = vec[current_idx].count()) {
                return_type.pointee = new_return_type;
                return_type.ptr_layers = count;
                return_type.type = SALT_TYPE_PTR;
            } else {
                return_type = new_return_type;
            }
            this->next();
        }
        
        else
            return ParserException(vec[current_idx], "expected type name after -> symbol");
    }   

    if (named_functions[function_name])
        return Exception (f_string("%d:%d: function %s already exists", function_identifier_token.line(), function_identifier_token.col(), function_name.c_str()).c_str());
    
    named_functions[function_name] = return_type;
    return std::make_unique<DeclarationAST>(function_identifier_token, std::move(args), return_type);

}

Result<Expression> Parser::parse_return() {
    int cur = this->current_idx;

    if (current().val() != TOK_RETURN)
        return ParserException(current(), "expected return keyword");

    this->next();

    int old_position = current_idx;

    Result<Expression> res = parse_expression();
    if (res.is_ok()) {
        return std::make_unique<ReturnAST>(vec[cur], std::move(res.unwrap()));
    }
    else { // clearly, the user meant to return void, since an expression was not found!
        this->current_idx = old_position;
        return std::make_unique<ReturnAST>(vec[cur], std::make_unique<ValExprAST>(vec[cur], int64_t(0), SALT_TYPE_VOID));
    }
}

Result<Expression> Parser::parse_deref() {
    if (current().val() != TOK_MUL)
        return ParserException(current(), "expected '*'");

    this->next();
    Result<Expression> ptr = parse_primary(); // parse primary since '*' is an unary operator. so it must bind more tightly than any binary operator
    if (ptr)
        return std::make_unique<DerefExprAST>(ptr.unwrap());
    else
        return ptr.unwrap_err();
}

/// @todo: to check if a function always returns a value, you can remember this
// a function will return a value iff the last statement is a return, or a conditional that is fully saturated with returns
Result<std::unique_ptr<FunctionAST>> Parser::parse_function() {
    // assume that the current token is TOK_FN
    auto decl_res = parse_declaration(); // consumes that token
    if (!decl_res)
        return decl_res.unwrap_err();

    std::unique_ptr<DeclarationAST> decl = decl_res.unwrap();

    // Check the return type, if none is specified then it's implicitly void, otherwise we want an arrow and a type
    if (vec[current_idx].val() != TOK_COLON)
        return ParserException(vec[current_idx], "expected \":\" after non-extern function declaration");
    
    int token_delta = this->next().delta; // move fd from ':'. if the function creation fails then go back this many steps
    std::vector<Expression> ret_vec;
    bool already_parsed_return = false;

    while (this->current_scope == 1) {
        bool should_parse_next_expression = current().val() != TOK_EOF;
        if (!should_parse_next_expression)
            break;

        Result<Expression> body_res = parse_expression();
        if (!body_res)
            return body_res.unwrap_err();
        Expression body = body_res.unwrap();

        if (already_parsed_return)
            print_warning(f_string("%d:%d: code after a return will be ignored", body->line(), body->col()));
        else
            ret_vec.push_back(std::move(body));

        if (ReturnAST* parsed_ret = ret_vec.back()->to_return()) 
            already_parsed_return = true;

    }

    // here we check if the function contains bad variables (like variables of type void)
    bool has_bad_args = false;
    for (const std::unique_ptr<VariableExprAST>& var : decl->args())
        if (var->type() == SALT_TYPE_VOID) {
            has_bad_args = true;
            break;
        }

    if (has_bad_args) {
        named_functions.erase(decl->name());
        return Exception(f_string("Function %s contains variable(s) of void type", decl->name()));
    }

    return std::make_unique<FunctionAST>(std::move(decl), std::move(ret_vec));
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
    auto decl = std::make_unique<DeclarationAST>(Token(TOK_NONE), std::vector<std::unique_ptr<VariableExprAST>>());
    print_fatal(f_string("%d:%d: Tried to parse top-level expression, this is unsupported", vec[current_idx].line(), vec[current_idx].col()));
    return Exception("very very very bad logic error");
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
        fail_ret_value = ParserException(vec[index_before_parse], expr_res.unwrap_err().what());


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
        if (salt::dbout.is_active())
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
        if (salt::dbout.is_active())
            generated_ir->print(llvm::errs());
        salt::dbout << " at: "
            << func->decl()->line()
            << ':'
            << func->decl()->col()
            << std::endl;
        return Result_e::OK;
    } else {
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
        if (salt::dbout.is_active())
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


ParserReturnType Parser::parse() {
    // for (int i = 0; i < 1; i++)
    try {
        Result<void> res;
        while (1) {
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
                break;
            case TOK_FN:
                res = handle_function();
                break;
            /*
            case TOK_NUMBER:
            case TOK_SUB:
            case TOK_REPEAT:
            case TOK_IF:
                res = handle_top_level_expr();
                break;
            */
            default: 
                res = handle_function();
                break;
            }

            if (!res) {
                print_error(res.unwrap_err().what());
            }
        }
    }

    catch (const ParserException& e) {
        std::cerr << e.what() << std::endl;
        if (can_go_next())
            this->next();
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
        + "[\""
        + (tok.has_data() ? tok.data() : "")
        + "\", "
        + std::to_string(tok.count())
        + "], "
        + str
        )) {}



