#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "operators.h"
#include "types.h"
#include "../common.h"
#include "frontendllvm.h"


class Variable {
private:
    std::string name_;
    const Type& type_;

public:
    Variable(const std::string& name, const Type& type) : name_(name), type_(type) {}
    const std::string& name() const {return name_;}
    const Type& type() const {return type_;}
    friend std::ostream& operator<<(std::ostream& os, const Variable& var);
};


// Credit where credit is due, this part is heavily "inspired" by the Kaleidoscope
// LLVM guide. Still, this file is part of my salt project and therefore under the
// gpl v3 license.

/// @todo: add types, not just "int"


// Base class for expression nodes in the AST
// Represents an expression of any kind.
class ExprAST {
protected:
    int line_;
    int col_;

public:
    virtual ~ExprAST() = default;

    // Generate LLVM IR for this node.
    virtual llvm::Value* code_gen() = 0;

    int line() const { return line_; }
    int col() const { return col_; }
};
typedef std::unique_ptr<ExprAST> Expression;


// Integer node, int literal such as "17"
class IntExprAST : public ExprAST {
private:
    int val_;

public:
    IntExprAST(const Token& tok, int val);
    virtual llvm::Value* code_gen() override;
};

// Variable name node
class VariableExprAST : public ExprAST {
private:
    Variable var_;

public:
    VariableExprAST(const Token& tok);
    const Variable& var() const;
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
};

// An expression that represents a called function.
class CallExprAST : public ExprAST {
private:
    std::string callee_;
    std::vector<Expression> args_;
public:
    CallExprAST(const Token& callee, std::vector<Expression> args);
    const std::string& callee() const;
    const std::vector<Expression>& args() const;
    virtual llvm::Value* code_gen() override;
};

class IfExprAST : public ExprAST {
private:
    Expression condition_;
    Expression true_expr_;
    Expression false_expr_;
public:
    IfExprAST(Expression cond, Expression true_expr, Expression false_expr) :
        condition_(std::move(cond)), true_expr_(std::move(true_expr)), false_expr_(std::move(false_expr)) {}
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
        line_(line), col_(col), loop_until_expr_(std::move(loop_until_expr)), loop_body_(std::move(loop_body)) {}
    const Expression& loop_until_expr() const ;
    const Expression& loop_body() const;
    int line() const;
    int col() const;
    llvm::Value* code_gen() override;
};

class DeclarationAST {
private:
    int line_;
    int col_;
    std::string name_;
    std::vector<Variable> args_;

public:
    DeclarationAST(const Token& tok, std::vector<Variable> args)
    : line_(tok.line()), col_(tok.col()), name_(tok.data()), args_(args) {}

    int line() const { return line_; }
    int col() const { return col_; }
    const std::string& name() const {return name_;}
    const std::vector<Variable>& args() const;
    llvm::Function* code_gen();
};

class FunctionAST {
private:
    std::unique_ptr<DeclarationAST> decl_;
    Expression body_;

public:
    FunctionAST(std::unique_ptr<DeclarationAST> decl, Expression body)
    : decl_(std::move(decl)), body_(std::move(body)) {}
    Expression toExprAST();

    const std::unique_ptr<DeclarationAST>& decl() const;
    const Expression& body() const;
    llvm::Function* code_gen();
};


