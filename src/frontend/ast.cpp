#include "ast.h"
#include <sstream>
#include "../common.h"
#include "irgenerator.h"
#include "types.h"

#define ASTCNDEBUG
#define RET_POISON_WITH_ERROR(type) do {\
    print_error(f_string("unsupported operation for `%s` type (line %d at function %s in compiler)", type->name.c_str(), __LINE__, __FUNCTION__));\
    return get_poison_value(type);\
} while (0)

#define RET_POISON_WITH_ERROR_VAL(val) do {\
    print_error(f_string("%d:%d: unsupported operation for `%s` type (line %d at function %s in compiler)", val->line(), val->col(), val->type()->name.c_str(), __LINE__, __FUNCTION__));\
    return get_poison_value(val->type());\
} while (0)

#define RET_POISON_WITH_ERROR_MSG(type, error) do {\
    print_error(error);\
    return get_poison_value(type);\
} while (0)

using namespace salt;

static llvm::Value* my_get_poison_value(const llvm::Type* type) {
    IRGenerator* gen = IRGenerator::get();
    if (type == llvm::Type::getVoidTy(*gen->context))
        return llvm::PoisonValue::get(llvm::Type::getInt64Ty(*gen->context));

    return llvm::PoisonValue::get(const_cast<llvm::Type*>(type));
}

static llvm::Value* get_poison_value(const salt::Type* type) {
    return my_get_poison_value(type->get());
}

// Make the variable printable with salt::to_string (for vectors).
std::ostream& operator<<(std::ostream& os, const VariableExprAST& var) {
    os << "Variable (type: " << var.type()->name << ", name: " << var.name() << ')';
    return os;
}

std::string ExprAST::ast_type() const {
    std::string my_type = "unknown type??";
    if (is_binary())
        my_type = "binary expr";
    else if (is_val())
        my_type = "val expr";
    else if (this->is_variable())
        my_type = "variable expr";
    else if (this->is_type())
        my_type = "type expr";
    else if (this->is_return())
        my_type = "return expr";
    else if (this->is_call())
        my_type = "call expr";

    return my_type;
}


ValExprAST::ValExprAST(const Token& tok, int64_t val, TypeInstance ti) {
    this->col_ = tok.col();
    this->line_ = tok.line();
    this->val_.i64 = val;
    this->ti_ = ti;
}

ValExprAST::ValExprAST(const Token& tok, double val, TypeInstance ti) {
    this->col_ = tok.col();
    this->line_ = tok.line();
    this->val_.f64 = val;
    this->ti_ = ti;
}

ValExprAST::ValExprAST(const Token& tok, std::string str, TypeInstance ti) {
    this->col_ = tok.col();
    this->line_ = tok.line();
    this->val_.i64 = 0;
    this->ti_ = ti;
    this->data_ = str;
}

VariableExprAST::VariableExprAST(const Token& tok, TypeInstance ti) : name_(tok.data()) {
    this->col_ = tok.col();
    this->line_ = tok.line();
    this->ti_ = ti;
    if (tok.val() != TOK_IDENT)
        print_fatal(f_string("Cannot create variable from token %s with data `%s`, expected identifier (TOK_IDENT)", tok.str().c_str(), tok.data()));
}

const std::string& VariableExprAST::name() const {
    return name_;
}


BinaryExprAST::BinaryExprAST(const Token& tok, Expression lhs, Expression rhs) {
    this->op_ = tok.val();
    this->lhs_ = std::move(lhs);
    this->rhs_ = std::move(rhs);
    this->col_ = lhs_->col();
    this->line_ = lhs_->line();
    
    // type promotion: the BinaryExpr will have the size of the largest of the types, if an implicit conversion is possible
    this->ti_.type = lhs_->type();
    this->ti_.pointee = lhs_->type();
    if (ti_.type->rank <= 1) {
        this->ti_.type = SALT_TYPE_ERROR;
        this->ti_.pointee = nullptr;
        return;
    }

    const salt::Type* rhs_type = rhs_->type();
    if (rhs_type->rank <= 1)
        this->ti_.type = SALT_TYPE_ERROR;
    else if (rhs_type->rank > this->ti_.type->rank) {
        this->ti_.type = rhs_type;
        this->ti_.pointee = rhs_->pointee();
    }

    if (this->ti_.type != SALT_TYPE_PTR)
        this->ti_.pointee = nullptr;

    salt::dboutv << f_string("Created new BinExprAST with %s x %s -> %s\n", lhs_->type()->name, rhs_->type()->name, this->ti_.type->name);
    
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



CallExprAST::CallExprAST(const Token& callee, std::vector<Expression> args, TypeInstance ti) :
    callee_(callee.data()), args_(std::move(args)) {
    this->col_ = callee.col();
    this->line_ = callee.line();
    this->ti_ = ti;
}

const std::string& CallExprAST::callee() const {
    return this->callee_;
}

const std::vector<Expression>& CallExprAST::args() const {
    return this->args_;
}

std::vector<Expression>& FunctionAST::body() {
    return this->body_;
}

const std::vector<std::unique_ptr<VariableExprAST>>& DeclarationAST::args() const {
    return this->args_;
}

std::unique_ptr<DeclarationAST>& FunctionAST::decl() {
    return this->decl_;
}

int RepeatAST::line() const {
    return this->line_;
}

int RepeatAST::col() const {
    return this->col_;
}

Expression& RepeatAST::loop_until_expr() {
    return loop_until_expr_;
}

Expression& RepeatAST::loop_body() {
    return loop_body_;
}


ReturnAST::ReturnAST(const Token& tok, Expression expr) : return_val(std::move(expr)) {
    this->line_ = tok.line();
    this->col_ = tok.col();
    this->ti_ = SALT_TYPE_VOID;
}



std::unique_ptr<ReturnAST> ExprAST::to_return() { return std::unique_ptr<ReturnAST>(is_return() ? static_cast<ReturnAST*>(this) : nullptr); }
std::unique_ptr<ValExprAST> ExprAST::to_val() { return std::unique_ptr<ValExprAST>(is_val() ? static_cast<ValExprAST*>(this) : nullptr); }
std::unique_ptr<BinaryExprAST> ExprAST::to_binary() { return std::unique_ptr<BinaryExprAST>(is_binary() ? static_cast<BinaryExprAST*>(this) : nullptr); }
std::unique_ptr<VariableExprAST> ExprAST::to_variable() { return std::unique_ptr<VariableExprAST>(is_variable() ? static_cast<VariableExprAST*>(this) : nullptr); }
std::unique_ptr<TypeExprAST> ExprAST::to_type() { return std::unique_ptr<TypeExprAST>(is_variable() ? static_cast<TypeExprAST*>(this) : nullptr); }

// For code generation to IR

using namespace llvm;

Value* ValExprAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    const llvm::Type* my_type = type()->get();

    // we return the biggest possible int size so we dont lose any data if the user actually does want a (u)i64
    if (my_type == llvm::Type::getInt64Ty(*gen->context))
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*gen->context), val_.i64, true);
    else if (my_type == llvm::Type::getDoubleTy(*gen->context))
        return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*gen->context), val_.f64);
    else if (my_type == llvm::PointerType::get(*gen->context, 0)) {

        // type is void*, only one case: null keyword
        if (ti_.pointee == SALT_TYPE_VOID && ti_.ptr_layers == 1) {
            Value* ptr_val = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*gen->context), val_.i64, false);
            return gen->builder->CreateIntToPtr(ptr_val, llvm::PointerType::get(*gen->context, 0));
        }

        // type is char*, meaning this is a string
        else if (ti_.pointee == SALT_TYPE_CHAR && ti_.ptr_layers == 1) {
            if (llvm::Value* ret_val = gen->named_strings[this->data()])
                return ret_val;
            else
                return (gen->named_strings[this->data()] = gen->builder->CreateGlobalStringPtr(this->data(), "str"));
        }

        else if (ti_.pointee) {
            std::string error = f_string("%s: wrong type (%s", __FUNCTION__, ti_.pointee->name.c_str());
            for (int i = 0; i < ti_.ptr_layers; i++)
                error += '*';
            error += ')';
            print_fatal(error);
        }

        else
            print_fatal(f_string("%s: VERY bad TypeInstance (type: %p, pointee: %p, ptr_layers: %d)", __FUNCTION__, ti_.type, ti_.pointee, ti_.ptr_layers));

        
    }
    else if (my_type == llvm::Type::getInt1Ty(*gen->context))
        return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*gen->context), val_.i64, false);
    else
        print_fatal(f_string("%s: wrong type (%s)", __FUNCTION__, type()->name.c_str()));

    return nullptr;
}

Value* VariableExprAST::code_gen() {

    IRGenerator* gen = IRGenerator::get();

    // if (this->var().type().name() != DEFAULT_TYPES[DT_INT].name())
    //     print_fatal("Types other than \"int\" NYI");
    for (const std::pair<std::string, Value*>& pair : gen->named_values) {
        salt::dboutv << f_string("Name %s: ptr %p\n", pair.first.c_str(), pair.second);
    }
    salt::dboutv << f_string("I am a variable of type %s and my name is %s\n", this->type()->name, this->name());

    Value* res = gen->named_values[this->name()];
    
    if (!res) {
        any_compile_error_occured = true;
        print_fatal(f_string("Unknown variable name: %s", this->name()));
        return nullptr; // Result(IRGeneratorException(this->line(), this->col(), "Unknown variable name: " + this->var_.name()));
    }



    return res;
}

Value* TypeExprAST::code_gen() {
    return nullptr;
}

TypeExprAST::TypeExprAST(const Token& tok) {
    this->line_ = tok.line();
    this->col_ = tok.col();

    if (tok.val() != TOK_TYPE)
        print_fatal(f_string("Attempted to create TypeInstance with token of type %s and data `%s`", tok.str(), tok.data()));

    ti_ = TypeInstance(
        tok.count() ? SALT_TYPE_PTR : salt::all_types[tok.data()],
        tok.count() ? salt::all_types[tok.data()] : nullptr,
        tok.count()
    );

    if (!ti_)
        ti_ = SALT_TYPE_ERROR;
}

TypeExprAST::TypeExprAST(const TypeInstance& ti) {
    if (!ti_)
        print_fatal("Invalid TypeInstance for TypeExprAST ctor");
    ti_ = ti;
}

Value* BinaryExprAST::code_gen() {

    IRGenerator* gen = IRGenerator::get();
    Value* left_proto = lhs_->code_gen();
    if (!left_proto)
        print_fatal("Bad LHS to BinaryExprAST::code_gen()");

    Value* right_proto = rhs_->code_gen();
    if (!right_proto && !rhs_->is_type())
        print_fatal("Bad RHS to BinaryExprAST::code_gen()");

    const salt::Type* new_type = lhs_->type();
    if (new_type->rank < rhs_->type()->rank)
        new_type = rhs_->type();

    salt::dboutv << f_string("BinExprAST has types %s and %s, converting to %s\n", lhs_->type()->name.c_str(), rhs_->type()->name.c_str(), new_type->name.c_str());


    Value* left = nullptr;
    Value* right = nullptr;
    
    // This is a trivial operation between two non-pointers, so convert both of them to the biggest type of the two
    // Or, it's not a pointer offset (that is, one of lhs/rhs is ptr, the other one is an integer)
    if (lhs_->pointee() == nullptr && rhs_->pointee() == nullptr && right_proto) {
        left = convert_implicit(left_proto, new_type->get(), lhs_->type()->is_signed);
        right = convert_implicit(right_proto, new_type->get(), rhs_->type()->is_signed);

        if (!left || !right) {
            print_error(f_string("unsupported conversion between `%s` and `%s` type", lhs_->type()->name.c_str(), rhs_->type()->name.c_str()));
            salt::dboutv << "Creating poison value for new type at address " << new_type << std::endl;
            return llvm::PoisonValue::get(const_cast<llvm::Type*>(new_type->get()));
        }
    }

    // Both operands are pointers, no conversion necessary
    else if (lhs_->pointee() != nullptr && rhs_->pointee() != nullptr) {
        left = left_proto;
        right = right_proto;
    }

    // Ok but at least it's not a pointer offset (add or sub) so no handling required
    else if (!!lhs_->ptr_layers() != !!rhs_->ptr_layers() && (op_ != TOK_ADD && op_ != TOK_SUB))
    {
        left = left_proto;
        right = right_proto;
    }

/*
    if (!left || !right) {
        salt::dboutv << "Creating poison value for new type at address " << new_type << std::endl;
        return llvm::PoisonValue::get(const_cast<llvm::Type*>(new_type->get()));
    }
*/



    // what kind of operands are we dealing with here?
    enum BinType {
        BIN_TYPE_INT,
        BIN_TYPE_UINT,
        BIN_TYPE_FLOAT,
        BIN_TYPE_PTR,
        BIN_TYPE_INVALID,
        BIN_TYPE_EXPLICIT_CAST
    };

    BinType bin_type;
    const llvm::Type* llvm_new_type = new_type->get();
    if (llvm_new_type->isPointerTy())
        bin_type = BIN_TYPE_PTR;
    else if (llvm_new_type->isFloatingPointTy())
        bin_type = BIN_TYPE_FLOAT;
    else if (llvm_new_type->isIntegerTy())
        if (new_type->is_signed)
            bin_type = BIN_TYPE_INT;
        else
            bin_type = BIN_TYPE_UINT;
    else
        bin_type = BIN_TYPE_INVALID;

    if (rhs_->is_type())
        bin_type = BIN_TYPE_EXPLICIT_CAST;
    
    salt::dboutv << "Decided bintype (" << int(bin_type) << ")!\n";

    // remember that for all bin_types except BIN_TYPE_PTR, BIN_TYPE_INVALID, left and right are not nullptr
    switch (this->op_) {

    /// @todo: mangle these names (like f"$Salt_addtmp_{lhs}_"
    /// @todo: add alignment requirements/auto-align pointers? 
    case TOK_ADD:
        switch (bin_type) {
        case BIN_TYPE_INT:
        case BIN_TYPE_UINT:
            return gen->builder->CreateAdd(left, right, "addtmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFAdd(left, right, "addtmp");
        case BIN_TYPE_PTR:
            // Try to parse this as a ptr addition if possible
            if (!left && !right) {

                Value* ptr_val = nullptr;
                Value* offset_val = nullptr;

                const TypeInstance& ti_left = lhs_->type_instance();
                const TypeInstance& ti_right = rhs_->type_instance();
                const TypeInstance* ti_ptr = nullptr;
                const TypeInstance* ti_offset = nullptr;

                ASSERT(ti_left.type != ti_right.type);
                if (ti_left.type == SALT_TYPE_PTR) {
                    ptr_val = left_proto;
                    offset_val = right_proto;
                    ti_ptr = &lhs_->type_instance();
                    ti_offset = &rhs_->type_instance();
                } else {
                    ptr_val = right_proto;
                    offset_val = left_proto;
                    ti_ptr = &rhs_->type_instance();
                    ti_offset = &lhs_->type_instance();
                }

                int offset_size = 0;
                if (ti_ptr->ptr_layers > 1)
                    offset_size = SALT_TYPE_PTR->get()->getPrimitiveSizeInBits() / 8;
                else
                    offset_size = ti_ptr->pointee->get()->getPrimitiveSizeInBits() / 8;

                if (!offset_val->getType()->isIntegerTy())
                    RET_POISON_WITH_ERROR_MSG(new_type, f_string("%d:%d: pointer offset must be an integer", rhs_->line(), rhs_->col()));

                offset_size = std::max(1, offset_size);
                offset_val = convert_implicit(offset_val, SALT_TYPE_SSIZE->get(), ti_offset->type->is_signed);
                offset_val = gen->builder->CreateMul(offset_val, llvm::ConstantInt::get(offset_val->getType(), offset_size), "offsettmp");
                if (salt::dboutv.is_active()) {
                    salt::dboutv << "Offset val: ";
                   offset_val->print(llvm::outs());
                }
                    
                return gen->builder->CreatePtrAdd(ptr_val, offset_val, "ptradd");

            }
            RET_POISON_WITH_ERROR(new_type);
            break;
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;
        
    case TOK_SUB:
        switch (bin_type) {
        case BIN_TYPE_INT:
        case BIN_TYPE_UINT:
            return gen->builder->CreateSub(left, right, "subtmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFSub(left, right, "subtmp");
        case BIN_TYPE_PTR:
            // Try to parse this as a ptr addition if possible
            if (!left && !right) {

                Value* ptr_val = nullptr;
                Value* offset_val = nullptr;

                const TypeInstance& ti_left = lhs_->type_instance();
                const TypeInstance& ti_right = rhs_->type_instance();
                const TypeInstance* ti_ptr = nullptr;
                const TypeInstance* ti_offset = nullptr;

                ASSERT(ti_left.type != ti_right.type);
                if (ti_left.type == SALT_TYPE_PTR) {
                    ptr_val = left_proto;
                    offset_val = right_proto;
                    ti_ptr = &lhs_->type_instance();
                    ti_offset = &rhs_->type_instance();
                }
                else {
                    ptr_val = right_proto;
                    offset_val = left_proto;
                    ti_ptr = &rhs_->type_instance();
                    ti_offset = &lhs_->type_instance();
                }

                int offset_size = 0;
                if (ti_ptr->ptr_layers > 1)
                    offset_size = SALT_TYPE_PTR->get()->getPrimitiveSizeInBits() / 8;
                else
                    offset_size = ti_ptr->pointee->get()->getPrimitiveSizeInBits() / 8;

                if (!offset_val->getType()->isIntegerTy())
                    RET_POISON_WITH_ERROR_MSG(new_type, f_string("%d:%d: pointer offset must be an integer", rhs_->line(), rhs_->col()));

                offset_size = std::max(1, offset_size);
                offset_val = convert_implicit(offset_val, SALT_TYPE_SSIZE->get(), ti_offset->type->is_signed);
                offset_val = gen->builder->CreateMul(offset_val, llvm::ConstantInt::get(offset_val->getType(), -offset_size));
                return gen->builder->CreatePtrAdd(ptr_val, offset_val, "ptrsub");
            }
            RET_POISON_WITH_ERROR(new_type);
            break;
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_MUL:
        switch (bin_type) {
        case BIN_TYPE_INT:
        case BIN_TYPE_UINT:
            return gen->builder->CreateMul(left, right, "multmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFMul(left, right, "multmp");
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_DIV:
        switch (bin_type) {
        case BIN_TYPE_INT:
            return gen->builder->CreateSDiv(left, right, "divtmp");
        case BIN_TYPE_UINT:
            return gen->builder->CreateUDiv(left, right, "udivtmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFDiv(left, right, "fdivtmp");
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;


    case TOK_LEFT_SHIFT:
        switch (bin_type) {
        case BIN_TYPE_INT:
        case BIN_TYPE_UINT:
            return gen->builder->CreateShl(left, right, "shltmp");
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_RIGHT_SHIFT:
        switch (bin_type) {
        case BIN_TYPE_INT:
            return gen->builder->CreateLShr(left, right, "shrtmp");
        case BIN_TYPE_UINT:
            return gen->builder->CreateAShr(left, right, "shrtmp");
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;


    // Create Integer Comparison Signed L/G Than, since right now all numbers are i32s.
    case TOK_LEFT_ANGLE:
        switch (bin_type) {
        case BIN_TYPE_INT:
            return gen->builder->CreateICmpSLT(left, right, "cmptmp");
        case BIN_TYPE_UINT:
            return gen->builder->CreateICmpULT(left, right, "cmptmp");
        case BIN_TYPE_FLOAT:
            // Create Floating Comparison Ordered Less Than (ordered: always return false if there is a NaN)
            return gen->builder->CreateFCmpOLT(left, right, "cmptmp");
        case BIN_TYPE_PTR: {
            Value* left_ptr_to_int = gen->builder->CreatePtrToInt(left, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            Value* right_ptr_to_int = gen->builder->CreatePtrToInt(right, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            return gen->builder->CreateICmpULT(left_ptr_to_int, right_ptr_to_int, "cmptmp");
        }
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_RIGHT_ANGLE:
        switch (bin_type) {
        case BIN_TYPE_INT:
            return gen->builder->CreateICmpSGT(left, right, "cmptmp");
        case BIN_TYPE_UINT:
            return gen->builder->CreateICmpUGT(left, right, "cmptmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFCmpOGT(left, right, "cmptmp");
        case BIN_TYPE_PTR: {
            Value* left_ptr_to_int = gen->builder->CreatePtrToInt(left, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            Value* right_ptr_to_int = gen->builder->CreatePtrToInt(right, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            return gen->builder->CreateICmpUGT(left_ptr_to_int, right_ptr_to_int, "cmptmp");
        }
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_EQUALS:
        switch (bin_type) {
        case BIN_TYPE_INT:
        case BIN_TYPE_UINT:
            return gen->builder->CreateICmpEQ(left, right, "cmptmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFCmpOEQ(left, right, "cmptmp");
        case BIN_TYPE_PTR: {
            Value* left_ptr_to_int = gen->builder->CreatePtrToInt(left, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            Value* right_ptr_to_int = gen->builder->CreatePtrToInt(right, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            return gen->builder->CreateICmpEQ(left_ptr_to_int, right_ptr_to_int, "cmptmp");
        }
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_EQUALS_LARGER:
        switch (bin_type) {
        case BIN_TYPE_INT:
            return gen->builder->CreateICmpSGE(left, right, "cmptmp");
        case BIN_TYPE_UINT:
            return gen->builder->CreateICmpUGE(left, right, "cmptmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFCmpOGE(left, right, "cmptmp");
        case BIN_TYPE_PTR: {
            Value* left_ptr_to_int = gen->builder->CreatePtrToInt(left, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            Value* right_ptr_to_int = gen->builder->CreatePtrToInt(right, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            return gen->builder->CreateICmpUGE(left_ptr_to_int, right_ptr_to_int, "cmptmp");
        }
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_EQUALS_SMALLER:
        switch (bin_type) {
        case BIN_TYPE_INT:
            return gen->builder->CreateICmpSLE(left, right, "cmptmp");
        case BIN_TYPE_UINT:
            return gen->builder->CreateICmpULE(left, right, "cmptmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFCmpOLE(left, right, "cmptmp");
        case BIN_TYPE_PTR: {
            Value* left_ptr_to_int = gen->builder->CreatePtrToInt(left, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            Value* right_ptr_to_int = gen->builder->CreatePtrToInt(right, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            return gen->builder->CreateICmpULE(left_ptr_to_int, right_ptr_to_int, "cmptmp");
        }
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_NOT_EQUALS:
        switch (bin_type) {
        case BIN_TYPE_INT:
        case BIN_TYPE_UINT:
            return gen->builder->CreateICmpNE(left, right, "cmptmp");
        case BIN_TYPE_FLOAT:
            return gen->builder->CreateFCmpONE(left, right, "cmptmp");
        case BIN_TYPE_PTR: {
            Value* left_ptr_to_int = gen->builder->CreatePtrToInt(left, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            Value* right_ptr_to_int = gen->builder->CreatePtrToInt(right, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            return gen->builder->CreateICmpNE(left_ptr_to_int, right_ptr_to_int, "cmptmp");
        }
        default:
            RET_POISON_WITH_ERROR(new_type);
        }
        break;

    case TOK_AS: {
        if (bin_type != BIN_TYPE_EXPLICIT_CAST)
            RET_POISON_WITH_ERROR_VAL(rhs_);
        salt::dboutv << "Attempting expl conversion!!\n";
        Value* cast_val = convert_explicit(left_proto, rhs_->type_instance().get(), rhs_->type()->is_signed);
        if (cast_val)
            return cast_val;
        else
            salt::dboutv << "Error: no cast_val!!\n";
        RET_POISON_WITH_ERROR_VAL(rhs_);
    }

    default:
        print_fatal("Found bad token " + Token(op_).str() + "in BinaryExprAST::code_gen()");
    }
}

Value* CallExprAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    Function* callee_fn = gen->mod->getFunction(this->callee());

    // Error handling: the function must exist, and be passed the correct number of arguments, and the arguments must be of the correct type.
    // Variadic functions will be added much later or possibly never.
    if (!callee_fn)
        print_fatal(IRGeneratorException(this->line(), this->col(), "no function exists named " + this->callee()));
    size_t arg_size = args().size();
    if (callee_fn->arg_size() != arg_size)
        print_fatal(IRGeneratorException(this->line(), this->col(),
            " Function named "
            + this->callee()
            + " takes "
            + std::to_string(callee_fn->arg_size())
            + " arguments, but "
            + std::to_string(arg_size)
            + " were provided")); 

    // Ok, this is a valid call. Populate the argument vector with the provided arguments.
    // Make sure to try converting the arguments to the correct type.
    std::vector<Value*> argv; 

    int i = 0;
    bool failed = false;
    for (auto itr = callee_fn->arg_begin(); itr != callee_fn->arg_end(); itr++) {
        Value* arg = args_[i]->code_gen();
        arg = convert_implicit(arg, const_cast<llvm::Type*>(itr->getType()), args_[i]->type()->is_signed);
        if (!arg) {
            failed = true;
            print_error(f_string("Function %s was called with bad argument types", callee_fn->getName().str().c_str()));
            arg = llvm::PoisonValue::get(const_cast<llvm::Type*>(itr->getType()));
        }
        i++;
        argv.push_back(arg);
    }
        
        

    if (!argv.empty() && !argv.back())
        print_fatal("argv.back() is nullptr in CallExprAST::code_gen()");

    // void expressions must not be named!
    if (callee_fn->getReturnType() == llvm::Type::getVoidTy(*gen->context))
        return gen->builder->CreateCall(callee_fn, argv);

    return gen->builder->CreateCall(callee_fn, argv, "calltmp");
}

Value* IfExprAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    Value* cond_val = condition_->code_gen();

    // Check for the type of cond_val

    // Try to convert cond_val to an i1
    cond_val = convert_implicit(cond_val, SALT_TYPE_BOOL->get(), false);
    if (!cond_val) {
        print_error(f_string("%d:%d: bad if condition", condition_->line(), condition_->col()));
        cond_val = PoisonValue::get(llvm::Type::getInt1Ty(*gen->context));
    }
    else salt::dboutv << f_string("Condval ptr is %p\n", cond_val);

    if (cond_val->getType() != llvm::Type::getInt1Ty(*gen->context))
        print_error("fuck 1");

    // compare the condition to (llvm::bool)0, and set cond_val to the opposite of the result.
    cond_val = gen->builder->CreateICmpNE(
        cond_val, ConstantInt::get(const_cast<llvm::Type*>(SALT_TYPE_BOOL->get()), 0, false), "ifcond");



    // Let fn be the current function that we're working with.
    Function* fn = gen->builder->GetInsertBlock()->getParent();

    // Now create BasicBlocks for both true_expr and false_expr.
    BasicBlock* true_expr_bb = BasicBlock::Create(*gen->context, "true_expr", fn);

    BasicBlock* false_expr_bb = BasicBlock::Create(*gen->context, "false_expr");
    BasicBlock* merge_bb = BasicBlock::Create(*gen->context, "ifcont"); // what happens after the if expression

    // CreateConditionBranch - creates a branching instruction that tests cond and branches to either true_expr or false_expr.
    gen->builder->CreateCondBr(cond_val, true_expr_bb, false_expr_bb);

    // Emit the true_expr value.
    // However first we must make sure they are both of the same type, thus choose the biggest one of them.
    const salt::Type* new_type = true_expr_->type();
    if (false_expr_->type()->rank > new_type->rank)
        new_type = false_expr_->type();

    

    gen->builder->SetInsertPoint(true_expr_bb);
    Value* true_expr_val = true_expr_->code_gen();
    true_expr_val = convert_implicit(true_expr_val, new_type->get(), new_type->is_signed);
    if (!true_expr_val) {
        true_expr_val = PoisonValue::get(const_cast<llvm::Type*>(new_type->get()));
        print_error(f_string("%d:%d: bad type for variable", true_expr_->line(), true_expr_->col()));
    }
    gen->builder->CreateBr(merge_bb); // make code to "return" from the if expression at the end of this block

    true_expr_bb = gen->builder->GetInsertBlock(); // save this block for later.

    // Emit the false_expr value.
    fn->insert(fn->end(), false_expr_bb);
    gen->builder->SetInsertPoint(false_expr_bb);
    Value* false_expr_val = false_expr_->code_gen();
    false_expr_val = convert_implicit(false_expr_val, new_type->get(), new_type->is_signed);
    if (!false_expr_val) {
        false_expr_val = PoisonValue::get(const_cast<llvm::Type*>(new_type->get()));
        print_error(f_string("%d:%d: bad type for variable", false_expr_->line(), false_expr_->col()));
    }
    gen->builder->CreateBr(merge_bb); // make code to "return" from the if expression at the end of this block

    false_expr_bb = gen->builder->GetInsertBlock(); // save this block for later.

    // Emit the merge block.
    fn->insert(fn->end(), merge_bb);
    gen->builder->SetInsertPoint(merge_bb);

    // Put the phi node in the merge block.
    // The phi node is a value that is currently unknown but will be assigned later
    // We need to do this because SSA

    PHINode* phi_node = gen->builder->CreatePHI(const_cast<llvm::Type*>(new_type->get()), 2, "iftmp");

    phi_node->addIncoming(true_expr_val, true_expr_bb);
    phi_node->addIncoming(false_expr_val, false_expr_bb);
    
    return phi_node;

}

Value* ReturnAST::code_gen() { 
    IRGenerator* gen = IRGenerator::get();
    if (return_val->type() == SALT_TYPE_VOID)
        return gen->builder->CreateRetVoid();
    else
        return gen->builder->CreateRet(return_val->code_gen());
}

Function* DeclarationAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();
    std::vector<llvm::Type*> parameter_types;
    for (const std::unique_ptr<VariableExprAST>& var : args())
        parameter_types.push_back(const_cast<llvm::Type*> (var->type()->get()));


    FunctionType* ft = FunctionType::get(const_cast<llvm::Type*>(this->type()->get()), parameter_types, false);

    Function* f = Function::Create(ft, Function::ExternalLinkage, this->name(), *gen->mod.get());


    // Name the arguments appropriately to make life easier for us in the future.

    int i = 0;
    for (Argument& Arg : f->args())
        Arg.setName((args()[i++])->name());

    return f;
}

Function* FunctionAST::code_gen() {
    IRGenerator* gen = IRGenerator::get();

    Function* f = gen->mod->getFunction(this->decl()->name());

    if (!f)
        f = this->decl()->code_gen();

    if (!f)
        print_fatal(IRGeneratorException(this->decl()->line(), this->decl()->col(), "In FunctionAST::code_gen(): failed DeclarationAST::code_gen()"));
    
    // Check that the function has not already been defined (it's ok if it has been declared though)
    if (!f->empty()) {
        print_error(f_string("%d:%d: redefinition of function %s", decl()->line(), decl()->col(), decl()->name().c_str()));
        return f;
    }

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
        salt::dbout << "arg.getName(): " << std::string(arg.getName()) << std::endl;
    }

    salt::dbout << "created declaration" << std::endl;

    if (Expression& ret_expr = this->body().back()) {
        Value* res = ret_expr->code_gen();
        const llvm::Type* expected_type = this->decl()->type()->get();
        const llvm::Type* actual_type = res->getType();
        const TypeInstance& expected_salt_type = this->decl()->type_instance();
        const TypeInstance& actual_salt_type = ret_expr->type_instance();
        bool should_return_void = expected_type == SALT_TYPE_VOID->get();
        
        if (expected_type != actual_type || expected_salt_type != actual_salt_type) {
            Value* attempted_conversion = convert_implicit(res, expected_type, this->decl()->type()->is_signed);
            // We can convert pointers to bool for example, but we cant convert a void* to an int*, or an int* to an int**
            // however we can convert any ptr to a void
            // remember that if both inp types are fully equal, we wouldnt need to check this
            if (attempted_conversion != nullptr && 
                (expected_salt_type.ptr_layers == 0                                                                 // if the expected type is not a pointer, but a ptr is convertible to it, then OK
                    || actual_salt_type.ptr_layers == 0                                                             // if the expected type is a pointer, but a non-ptr is convertible to it, then OK
                    || (expected_salt_type.ptr_layers == 1 && expected_salt_type.pointee == SALT_TYPE_VOID))) {     // if the expected type is a void-ptr, then any ptr is convertible to it, then OK
                res = attempted_conversion;
            } 
            else {
                print_error(f_string("function `%s` returns %s when %s was expected",
                    f->getName().str().c_str(),
                    actual_salt_type.str().c_str(),
                    expected_salt_type.str().c_str()));
                if (!expected_type || expected_type == llvm::Type::getVoidTy(*gen->context)) {
                        // Do nothing
                }
                else {
                    res = llvm::PoisonValue::get(const_cast<llvm::Type*>(this->decl()->type()->get()));
                }
            }
        }

        // Create a return if res is not of void type.
        // If res is of the wrong type, this will be a poison value
        if (!should_return_void)
            gen->builder->CreateRet(res);
        else
            gen->builder->CreateRetVoid();

        salt::dbout << "created return" << std::endl;

        std::string error_result = "llvm error: ";
        llvm::raw_string_ostream error_os = llvm::raw_string_ostream(error_result);
        bool errors_found = verifyFunction(*f, &error_os);
        error_os << '\n';

        if (errors_found)
            print_fatal(error_result);
    } else {
        salt::dbout << "function " << this->decl()->name() << " does not have body" << std::endl;
    }
    salt::dbout << f_string("successfully created function %s\n", this->decl()->name().c_str());
    return f;
}

/// @todo: fix
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

/// @todo: make bool -> int conversion explicit
/// while keeping int -> bool implicit
Value* salt::convert_implicit(llvm::Value* value, const llvm::Type* _type, bool is_signed) {
    llvm::Type* current_type = const_cast<llvm::Type*>(value->getType());
    IRGenerator* gen = IRGenerator::get();
    llvm::Type* type = const_cast<llvm::Type*>(_type);

    // a value can always be implicitly converted into a value of the same type
    // and LLVM doesn't care if an integer is signed or unsigned, thus u32 = i32 for example
    if (current_type == type)
        return value;

    // first check if the dest type is bool, in this case we need to do a special conversion (that is, cmp with 0)
    // then, "if X" will only accept X of bool type officially
    if (type == SALT_TYPE_BOOL->get()) {
        if (current_type->isIntegerTy())
            return gen->builder->CreateICmpNE(value, llvm::ConstantInt::get(current_type, 0, false));
        else if (current_type->isFloatingPointTy())
            return gen->builder->CreateFCmpONE(value, llvm::ConstantFP::get(current_type, 0));
        else if (current_type->isPointerTy()) {
            Value* ptr_val = gen->builder->CreatePtrToInt(value, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()));
            return gen->builder->CreateICmpNE(ptr_val, llvm::ConstantInt::get(const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()), 0, false), "ptrtobool");
        }
    }

    // no valid binary op between 2 ptrs... <-- wrong! for example <, >, ==, != etc
    // if (current_type == type && current_type == SALT_TYPE_PTR->get())
    //    return nullptr;
    
    // now we know the types are different
    
    // Current type is integral
    if (current_type->isIntegerTy()) {
        // New type is integral
        if (type->isIntegerTy()) {
            if (is_signed)
                return gen->builder->CreateSExtOrTrunc(value, type, "intconvtemp");
            else
                return gen->builder->CreateZExtOrTrunc(value, type, "uintconvtemp");
        
        // New type is float type
        } else if (type->isFloatingPointTy()) {
            // Signed int -> float type
            if (is_signed)
                return gen->builder->CreateSIToFP(value, type, "intfloatconvtemp");
            // Unsigned int -> float type
            else
                return gen->builder->CreateUIToFP(value, type, "uintfloatconvtemp");
        
        // New type is ptr type
        // Don't convert this to a ptr
        } else if (type->isPointerTy()) {
            return nullptr;


        // An integral type cannot implicitly convert to this type
        } else {
            return nullptr;
        }

    // Current type is floating pt type
    } else if (current_type->isFloatingPointTy()) {
        // New type is float, current type must be double
        if (type->isFloatTy()) {
            return gen->builder->CreateFPTrunc(value, type, "fptrunctemp");

        // New type is double, current must be float
        } else if (type->isDoubleTy()) {
            return gen->builder->CreateFPExt(value, type, "fpexttemp");

        // New type is int
        } else if (type->isIntegerTy()) {
            // Signed int -> float type
            if (is_signed)
                return gen->builder->CreateFPToSI(value, type, "floatintconvtemp");
            // Unsigned int -> float type
            else
                return gen->builder->CreateFPToUI(value, type, "floatuintconvtemp");

            // A floating type cannot implicitly convert to this type
        }
        else {
            return nullptr;
        }

    
    }

    // Current type can't be implicitly converted to anything
    return nullptr;

}

llvm::Value* salt::convert_explicit(llvm::Value* val, const llvm::Type* _type, bool is_signed) {
    IRGenerator* gen = IRGenerator::get();
    llvm::Type* type = const_cast<llvm::Type*>(_type);

    if (llvm::Value* imp = convert_implicit(val, type, is_signed))
        return imp;

    if (type == SALT_TYPE_VOID->get())
        return llvm::PoisonValue::get(const_cast<llvm::Type*>(SALT_TYPE_VOID->get()));

    if (val->getType()->isIntegerTy() && type->isPointerTy()) {
        llvm::Value* extended_val = nullptr;
        if (is_signed)
            extended_val = gen->builder->CreateSExtOrTrunc(val, const_cast<llvm::Type*>(SALT_TYPE_SSIZE->get()), "inttossize");
        else
            extended_val = gen->builder->CreateZExtOrTrunc(val, const_cast<llvm::Type*>(SALT_TYPE_USIZE->get()), "uinttousize");

        return gen->builder->CreateIntToPtr(extended_val, const_cast<llvm::Type*>(SALT_TYPE_PTR->get()), "sizetoptr");
    }

    return nullptr;
}


/*
// New type is ptr type
        } else if (type->isPointerTy()) {
            // All signed offsets are allowed
            if (is_signed) {
                TODO();
            }
            // Unsigned int -> float type
            else if (current_type == llvm::IntegerType::get(*gen->context, IRGenerator::get()->mod->getDataLayout().getPointerSizeInBits()))
                return gen->builder->CreateUIToFP(value, type, "uintfloatconvtemp");

        // An integral type cannot implicitly convert to this type
        }
*/