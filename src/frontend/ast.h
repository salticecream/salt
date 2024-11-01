#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "operators.h"
#include "types.h"
#include "../common.h"
#include "frontendllvm.h"
#include "types.h"


// Credit where credit is due, this part is heavily "inspired" by the Kaleidoscope
// LLVM guide. Still, this file is part of my salt project and therefore under the
// gpl v3 license.

class ValExprAST;
class VariableExprAST;
class ReturnAST;
class BinaryExprAST;
class TypeExprAST;
class DerefExprAST;
class CallExprAST;

// Base class for expression nodes in the AST
// Represents an expression of any kind.

class ExprAST {
protected:
    int line_;
    int col_;
    TypeInstance ti_;
public:
    virtual ~ExprAST() = default;

    // Generate LLVM IR for this node.
    virtual llvm::Value* code_gen() = 0;

    int line() const                            { return line_; }
    int col() const                             { return col_; }
    const salt::Type* type() const              { return ti_.type; }
    const salt::Type* pointee() const           { return ti_.pointee; }
    TypeInstance& type_instance()               { return ti_; }
    int ptr_layers() const                      { return ti_.ptr_layers; };
    void set_type(TypeInstance type)            { this->ti_ = type; }
    virtual bool is_val() const                 { return false; }
    virtual bool is_return() const              { return false; }
    virtual bool is_variable() const            { return false; }
    virtual bool is_binary() const              { return false; }
    virtual bool is_type() const                { return false; }
    virtual bool is_deref() const               { return false; }
    virtual bool is_call() const                { return false; }
    virtual bool is_if() const                  { return false; }
    std::unique_ptr<ReturnAST> to_return();         // convert this to return expr if possible
    std::unique_ptr<ValExprAST> to_val();           // convert this to val expr if possible
    std::unique_ptr<VariableExprAST> to_variable(); // convert this to variable expr if possible
    std::unique_ptr<BinaryExprAST> to_binary();     // convert this to binary expr if possible
    std::unique_ptr<TypeExprAST> to_type();         // convert this to type expr if possible
    std::string ast_type() const;
};
typedef std::unique_ptr<ExprAST> Expression;


// Value node for literals
class ValExprAST : public ExprAST {
private:
    union {
        int64_t i64;
        double f64;
    }
    val_;
    std::string data_;

public:
    ValExprAST(const Token& tok, int64_t val, TypeInstance ti = SALT_TYPE_LONG);
    ValExprAST(const Token& tok, double val, TypeInstance ti = SALT_TYPE_DOUBLE);
    ValExprAST(const Token& tok, std::string str, TypeInstance ti = TypeInstance(SALT_TYPE_CHAR, 1));
    virtual llvm::Value* code_gen() override;
    virtual bool is_val() const override { return true; }
    std::string& data() { return data_; }
};

// Variable name node
class VariableExprAST : public ExprAST {
private:
    const std::string name_;

public:
    VariableExprAST(const Token& tok, TypeInstance ti = SALT_TYPE_LONG);
    const std::string& name() const;
    virtual bool is_variable() const override { return true; }
    virtual llvm::Value* code_gen() override;
};

// A binary expression, like a + b.
class BinaryExprAST : public ExprAST {
private:
    Token_e op_;
    Expression lhs_;
    Expression rhs_;
public:
    BinaryExprAST(const Token& op, Expression lhs, Expression rhs);
    const Expression& lhs() const;
    const Expression& rhs() const;
    Token_e op() const;
    virtual llvm::Value* code_gen() override;
    virtual bool is_binary() const override { return true; }
};

// An expression that represents a called function.
class CallExprAST : public ExprAST {
private:
    std::string callee_;
    std::vector<Expression> args_;
public:
    CallExprAST(const Token& callee, std::vector<Expression> args, TypeInstance ti = SALT_TYPE_LONG);
    const std::string& callee() const;
    const std::vector<Expression>& args() const;
    virtual bool is_call() const override { return true; }
    virtual llvm::Value* code_gen() override;
};

class IfExprAST : public ExprAST {
private:
    Expression condition_;
    Expression true_expr_;
    Expression false_expr_;
public:
    virtual bool is_if() const override { return true; }
    IfExprAST(Expression cond, Expression true_expr, Expression false_expr, TypeInstance ti = SALT_TYPE_LONG) :
        condition_(std::move(cond)), true_expr_(std::move(true_expr)), false_expr_(std::move(false_expr)) { ti_ = ti; }
    llvm::Value* code_gen() override;
};

class RepeatAST : public ExprAST {
private:
    int line_;
    int col_;
    Expression loop_until_expr_;
    Expression loop_body_;

public:
    RepeatAST(int line, int col, Expression loop_until_expr, Expression loop_body) :
        line_(line), col_(col), loop_until_expr_(std::move(loop_until_expr)), loop_body_(std::move(loop_body)) { ti_ = SALT_TYPE_VOID; }
    Expression& loop_until_expr();
    Expression& loop_body();
    int line() const;
    int col() const;
    llvm::Value* code_gen() override;
};

class TypeExprAST : public ExprAST {
public:
    virtual bool is_type() const override { return true; }
    virtual llvm::Value* code_gen() override;
    TypeExprAST(const TypeInstance& ti);
    TypeExprAST(const Token& tok);
};

class DeclarationAST {
private:
    int line_;
    int col_;
    std::string name_;
    std::vector<std::unique_ptr<VariableExprAST>> args_;
    TypeInstance ti_;

public:
    DeclarationAST(const Token& tok, std::vector<std::unique_ptr<VariableExprAST>> args, TypeInstance ti = SALT_TYPE_VOID)
    : line_(tok.line()), col_(tok.col()), name_(tok.data()), args_(std::move(args)) { ti_ = ti; }

    int line() const { return line_; }
    int col() const { return col_; }
    const std::string& name() const {return name_;}
    const std::vector<std::unique_ptr<VariableExprAST>>& args() const;
    const salt::Type* type() const { return ti_.type; } // return type of this function
    TypeInstance& type_instance() { return ti_; }
    llvm::Function* code_gen();
};

class FunctionAST {
private:
    std::unique_ptr<DeclarationAST> decl_;
    std::vector<Expression> body_;

public:
    FunctionAST(std::unique_ptr<DeclarationAST> decl, std::vector<Expression> body)
    : decl_(std::move(decl)), body_(std::move(body)) {}

    std::unique_ptr<DeclarationAST>& decl();
    std::vector<Expression>& body();
    llvm::Function* code_gen();
};

class ReturnAST : public ExprAST {
public:
    Expression return_val;
    ReturnAST(const Token& tok, Expression expr);
    virtual bool is_return() const override { return true; }
    llvm::Value* code_gen() override;
};

namespace salt {
    // Returns a llvm::Value* corresponding to value converted to type, or nullptr if this is not possible to do implicitly.
    // You need to specify both for the old and new values if they are signed or not.
    // Essentially only useful for converting numeric values to the correct type in expressions.
    llvm::Value* convert_implicit(llvm::Value* value, const llvm::Type* type, bool is_signed);

    // Returns a llvm::Value* corresponding to value converted to type, or nullptr if this is not possible to do explicitly.
    // You need to specify both for the old and new values if they are signed or not.
    // A stronger version of convert_implicit, it can cast anything to void, and convert bool to integral types.
    llvm::Value* convert_explicit(llvm::Value* value, const llvm::Type* type, bool is_signed);
};
