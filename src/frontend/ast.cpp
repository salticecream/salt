#include "ast.h"
#include <sstream>
#include "../common.h"
#include "irgenerator.h"

#define ASTCNDEBUG
using namespace salt;

// Make the variable printable with salt::to_string (for vectors).
std::ostream& operator<<(std::ostream& os, const Variable& var) {
    os << "Variable (type: " << var.type().name() << ", name: " << var.name() << ')';
    return os;
}



IntExprAST::IntExprAST(const Token& tok, int val) {
    this->col_ = tok.col();
    this->line_ = tok.line();
    this->val_ = val;
#ifndef NDEBUG
    if (std::to_string(this->val_) != tok.data())
        std::cerr << this->line_
            << ':'
            << this->col_
            << " warning: IntExprAST was created with int value " 
            << this->val_ 
            << " but the token's string data is `" 
            << tok.data()
            << '`'
            << std::endl;
#endif
}

VariableExprAST::VariableExprAST(const Token& tok) : var_(Variable(tok.data(), DEFAULT_TYPES[DT_INT])) {
    this->col_ = tok.col();
    this->line_ = tok.line();
    if (tok.val() != TOK_IDENT)
        throw salt::Exception("tok.val() was not TOK_IDENT");
}


const Variable& VariableExprAST::var() const {
    return this->var_;
}

BinaryExprAST::BinaryExprAST(const Token& tok, Expression lhs, Expression rhs) {
    this->op_ = tok.val();
    this->lhs_ = std::move(lhs);
    this->rhs_ = std::move(rhs);
    this->col_ = tok.col();
    this->line_ = tok.line();
}

const Expression& BinaryExprAST::lhs() const {
    return this->lhs_;
}

const Expression& BinaryExprAST::rhs() const {
    return this->rhs_;
}

Token_e BinaryExprAST::op() const {
    return this->op_;
}



CallExprAST::CallExprAST(const Token& callee, std::vector<Expression> args) :
    callee_(callee.data()), args_(std::move(args)) {
    this->col_ = callee.col();
    this->line_ = callee.line();
}

const std::string& CallExprAST::callee() const {
    return this->callee_;
}

const std::vector<Expression>& CallExprAST::args() const {
    return this->args_;
}

std::unique_ptr<ExprAST> FunctionAST::toExprAST() {
    return std::move(body_);
}

const Expression& FunctionAST::body() const {
    return this->body_;
}

const std::vector<Variable>& DeclarationAST::args() const {
    return this->args_;
}

const std::unique_ptr<DeclarationAST>& FunctionAST::decl() const {
    return this->decl_;
}

int RepeatAST::line() const {
    return this->line_;
}

int RepeatAST::col() const {
    return this->col_;
}

const Expression& RepeatAST::loop_until_expr() const {
    return loop_until_expr_;
}

const Expression& RepeatAST::loop_body() const {
    return loop_body_;
}





// For code generation to IR

using namespace llvm;

Value* IntExprAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen->context), val_, true);
}

Value* VariableExprAST::code_gen() {

    IRGenerator* gen = IRGenerator::get();

    if (this->var_.type().name() != DEFAULT_TYPES[DT_INT].name())
        throw salt::Exception("Types other than \"int\" NYI");

    Value* res = gen->named_values[this->var_.name()];
    if (!res) {
        any_compile_error_occured = true;
        TODO();
        return nullptr; // Result(IRGeneratorException(this->line(), this->col(), "Unknown variable name: " + this->var_.name()));
    }



    return res;
}

Value* BinaryExprAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    Value* left = lhs_->code_gen();
    if (!left)
        throw salt::Exception("Bad LHS to BinaryExprAST::code_gen()");

    Value* right = rhs_->code_gen();
    if (!right)
        throw salt::Exception("Bad RIGHT to BinaryExprAST::code_gen()");
    
    switch (this->op_) {

    /// @todo: mangle the fuck out of these names (like f"$Salt_addtmp_{lhs}_"
    case TOK_ADD:
        return gen->builder->CreateAdd(left, right, "addtmp");
    case TOK_SUB:
        return gen->builder->CreateSub(left, right, "subtmp");
    case TOK_MUL:
        return gen->builder->CreateMul(left, right, "multmp");
    case TOK_DIV:
        return gen->builder->CreateSDiv(left, right, "divtmp");
    case TOK_LEFT_SHIFT:
        return gen->builder->CreateShl(left, right, "shltmp");
    case TOK_RIGHT_SHIFT:
        // Logical shift right because right now all numbers are i32s.
        return gen->builder->CreateLShr(left, right, "shrtmp");

    // Create Integer Comparison Signed L/G Than, since right now all numbers are i32s.
    case TOK_LEFT_ANGLE:
        return gen->builder->CreateICmpSLT(left, right, "lcmptmp");
    case TOK_RIGHT_ANGLE:
        return gen->builder->CreateICmpSGT(left, right, "rcmptmp");
    case TOK_EQUALS:
        return gen->builder->CreateICmpEQ(left, right, "eqcmptmp");
    case TOK_EQUALS_LARGER:
        return gen->builder->CreateICmpSGE(left, right, "reqcmptmp");
    case TOK_EQUALS_SMALLER:
        return gen->builder->CreateICmpSLE(left, right, "leqcmptmp");
    case TOK_NOT_EQUALS:
        return gen->builder->CreateICmpNE(left, right, "neqcmptmp");
    default:
        throw salt::Exception("Found bad token " + Token(op_).str() + "in BinaryExprAST::code_gen()");
    }
}

Value* CallExprAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    Function* callee_fn = gen->mod->getFunction(this->callee());

    // Error handling: the function must exist, and be passed the correct number of arguments.
    // Variadic functions will be added much later or possibly never.
    if (!callee_fn)
        throw IRGeneratorException(this->line(), this->col(), "no function exists named " + this->callee());
    size_t arg_size = args().size();
    if (callee_fn->arg_size() != arg_size)
        throw IRGeneratorException(this->line(), this->col(),
            " Function named "
            + this->callee()
            + " takes "
            + std::to_string(callee_fn->arg_size())
            + " arguments, but "
            + std::to_string(arg_size)
            + " were provided"); 

    // Ok, this is a valid call. Populate the argument vector with the provided arguments.
    std::vector<Value*> argv; 
    for (int i = 0; i < arg_size; i++)
        argv.push_back(args_[i]->code_gen());
        

    if (!argv.empty() && !argv.back())
        throw salt::Exception("argv.back() is nullptr in CallExprAST::code_gen()");

    return gen->builder->CreateCall(callee_fn, argv, "calltmp");
}

Value* IfExprAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    Value* cond_val = condition_->code_gen();

    // Check for the type of cond_val
    llvm::Type* cv_type = cond_val->getType();

    if (cond_val->getType() == llvm::Type::getInt32Ty(*gen->context)) // cond_val is an integer
        cond_val = gen->builder->CreateICmpNE(
            cond_val, ConstantInt::get(llvm::Type::getInt32Ty(*gen->context), 0, true), "ifcond");
    else if (cv_type == llvm::Type::getInt1Ty(*gen->context))
        // compare the condition to (llvm::bool)0, and set cond_val to the opposite of the result.
        cond_val = gen->builder->CreateICmpNE(
            cond_val, ConstantInt::get(llvm::Type::getInt1Ty(*gen->context), 0, false), "ifcond");
    else
        throw std::exception("Wrong type for comparison, only i1 and i32 supported");

    // Let fn be the current function that we're working with.
    Function* fn = gen->builder->GetInsertBlock()->getParent();

    // Now create BasicBlocks for both true_expr and false_expr.
    BasicBlock* true_expr_bb = BasicBlock::Create(*gen->context, "true_expr", fn);

    BasicBlock* false_expr_bb = BasicBlock::Create(*gen->context, "false_expr");
    BasicBlock* merge_bb = BasicBlock::Create(*gen->context, "ifcont"); // what happens after the if expression

    // CreateConditionBranch - creates a branching instruction that tests cond and branches to either true_expr or false_expr.
    gen->builder->CreateCondBr(cond_val, true_expr_bb, false_expr_bb);

    // Emit the true_expr value.
    gen->builder->SetInsertPoint(true_expr_bb);
    Value* true_expr_val = true_expr_->code_gen();
    gen->builder->CreateBr(merge_bb); // "return" from the if expression

    true_expr_bb = gen->builder->GetInsertBlock(); // ?????????? the fuck

    // Emit the false_expr value.
    fn->insert(fn->end(), false_expr_bb);
    gen->builder->SetInsertPoint(false_expr_bb);
    Value* false_expr_val = false_expr_->code_gen();
    gen->builder->CreateBr(merge_bb); // "return" from the if expression

    false_expr_bb = gen->builder->GetInsertBlock(); // ?????????? the fuck

    // Emit the merge block.
    fn->insert(fn->end(), merge_bb);
    gen->builder->SetInsertPoint(merge_bb);

    // Put the phi node in the merge block.
    // The phi node is a value that is currently unknown but will be assigned later
    // We need to do this because SSA
    PHINode* phi_node = gen->builder->CreatePHI(llvm::Type::getInt32Ty(*gen->context), 2, "iftmp"); 

    phi_node->addIncoming(true_expr_val, true_expr_bb);
    phi_node->addIncoming(false_expr_val, false_expr_bb);
    
    return phi_node;

}



/// @todo: add types
Function* DeclarationAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    std::vector<llvm::Type*> ints(args().size(), llvm::Type::getInt32Ty(*gen->context));

    FunctionType* ft = FunctionType::get(llvm::Type::getInt32Ty(*gen->context), ints, false);

    Function* f = Function::Create(ft, Function::ExternalLinkage, this->name(), *gen->mod.get());


    // Name the arguments appropriately to make life easier for us in the future.

    unsigned Idx = 0;
    for (auto& Arg : f->args()) {
        Arg.setName((args_[Idx]).name());
        std::cout << "naming arguments: Arg.getName() is " << std::string(Arg.getName()) << std::endl;
    }

    return f;
}

Function* FunctionAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();

    Function* f = gen->mod->getFunction(this->decl()->name());

    if (!f)
        f = this->decl()->code_gen();

    if (!f)
        throw IRGeneratorException(this->decl()->line(), this->decl()->col(), "Failed DeclarationAST::code_gen()");
    
    // Check that the function has not already been defined (it's ok if it has been declared though)
    if (!f->empty())
        throw IRGeneratorException(this->decl()->line(), this->decl()->col(),
            "cannot redefine function " + this->decl()->name());

    // Tell the LLVM builder to generate code inside this block (the function).
    // Control flow comes later.
    BasicBlock* bb = BasicBlock::Create(*gen->context, "entry", f);
    gen->builder->SetInsertPoint(bb);

    // Since we are in a new scope, clear named_values.
    // Not the best way of doing it, but for simplicity this is the way it's going to be done
    gen->named_values.clear();

    // Now we populate named_values with the args that are in scope.
    for (auto& arg : f->args()) {
        gen->named_values[std::string(arg.getName())] = &arg;
        std::cout << "arg.getName(): " << std::string(arg.getName()) << std::endl;
    }

    std::cout << "created declaration" << std::endl;

    if (this->body()) {
        Value* res = this->body()->code_gen();

        gen->builder->CreateRet(res);

        std::cout << "created return" << std::endl;

        std::cout << "verifyfunction: ";
        bool errors_found = verifyFunction(*f, &llvm::outs());
        if (!errors_found)
            // optimize the function
            gen->fn_pass_mgr->run(*f, *gen->fn_analysis_mgr);
        std::cout << (errors_found ? "an error was found" : "") << std::endl;
    } else {
        std::cout << "function " << this->decl()->name() << " does not have body" << std::endl;
    }
    return f;
}

llvm::Value* RepeatAST::code_gen() {
    
    // initialize variables
    IRGenerator* gen = IRGenerator::get();
    Value* loop_ctr = ConstantInt::get(llvm::Type::getInt32Ty(*gen->context), 0, true);
    Value* step_val = ConstantInt::get(llvm::Type::getInt32Ty(*gen->context), 1, true);
    Value* loop_until_val = loop_until_expr_->code_gen();
    Function* fn = gen->builder->GetInsertBlock()->getParent();

    BasicBlock* preheader_bb = gen->builder->GetInsertBlock();
    BasicBlock* loop_bb = BasicBlock::Create(*gen->context, "loop", fn);

    // we need to create an explicit fallthrough from preheader_bb to loop_bb
    gen->builder->CreateBr(loop_bb);
    gen->builder->SetInsertPoint(loop_bb);

    PHINode* variable = gen->builder->CreatePHI(llvm::Type::getInt32Ty(*gen->context), 2, "$loop_ctr"); 
    variable->addIncoming(loop_ctr, preheader_bb); 
 
    // add the body of the expr to the loop
    loop_body_->code_gen(); 
    
    Value* next_val = gen->builder->CreateAdd(loop_ctr, step_val, "$loop_next_val"); 
    Value* end_cond = gen->builder->CreateICmpSGE(loop_ctr, loop_until_val, "$loop_end_cond"); 
    

    BasicBlock* loop_end_bb = gen->builder->GetInsertBlock(); 
    BasicBlock* after_bb = BasicBlock::Create(*gen->context, "after_loop", fn); 
    gen->builder->CreateCondBr(end_cond, loop_bb, after_bb); 
    
    // set the builder to print code after the loop (we are done).
    gen->builder->SetInsertPoint(after_bb); 

    variable->addIncoming(next_val, loop_end_bb); 
    
    // repeat loops should return void:
    // return Constant::getNullValue(llvm::Type::getVoidTy(*gen->context));
    // But not yet
    return Constant::getNullValue(llvm::Type::getInt32Ty(*gen->context));
    

}