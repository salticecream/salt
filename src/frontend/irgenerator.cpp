#include "irgenerator.h"
#include <vector>
#include <cstdarg>
#include <iostream>
#include "types.h"

/*
* Defines the IRGenerator class, which generates and optimizes LLVM IR.
* If you were looking for the IR generation code, go to ast.cpp
* 
* Due to reasons, the context is outside of the IRGenerator but should still be referred to using the IRGenerator
*/


IRGenerator* IRGenerator::instance = nullptr;

// GLOBAL variable, use IRGenerator->context instead
std::unique_ptr<llvm::LLVMContext> global_context = std::make_unique<llvm::LLVMContext>();

IRGenerator* IRGenerator::get() {
	return instance ? instance : instance = new IRGenerator();
}

void IRGenerator::destroy() {
	if (instance)
		delete instance;
	instance = nullptr;
}

IRGenerator::IRGenerator() : context(global_context) {

	// For code generation
	this->builder = std::make_unique<llvm::IRBuilder<>>(*this->context);
	this->mod = std::make_unique<llvm::Module>("salt", *this->context);
	this->named_values = { {}, {} }; // the first {} is for global scope, the second {} is for current function scope.
	this->named_strings = {};
	this->named_functions = {};


	// For optimization and whatnot
	pass_builder = std::make_unique<llvm::PassBuilder>(); // to be set later!
	loop_analysis_mgr = std::make_unique<llvm::LoopAnalysisManager>();
	legacy_fn_pass_mgr = std::make_unique<llvm::legacy::FunctionPassManager>(this->mod.get());
	fn_pass_mgr = std::make_unique<llvm::FunctionPassManager>();
	fn_analysis_mgr = std::make_unique<llvm::FunctionAnalysisManager>();
	cgscc_analysis_mgr = std::make_unique<llvm::CGSCCAnalysisManager>();
	module_analysis_mgr = std::make_unique<llvm::ModuleAnalysisManager>();
	module_pass_mgr = std::make_unique<llvm::ModulePassManager>();
	pass_instrumentation_callbacks = std::make_unique<llvm::PassInstrumentationCallbacks>();
	std_instrumentations = std::make_unique<llvm::StandardInstrumentations>(*context, /*DebugLogging = */ true);
	std_instrumentations->registerCallbacks(*pass_instrumentation_callbacks, module_analysis_mgr.get());
	

	// Add optimization passes
	fn_pass_mgr->addPass(llvm::InstCombinePass());							// optimize calculations
	fn_pass_mgr->addPass(llvm::ReassociatePass());							// reassociate expressions
	fn_pass_mgr->addPass(llvm::GVNPass());									// eliminate common subexpressions so they only need to be calculated once
	fn_pass_mgr->addPass(llvm::SimplifyCFGPass());							// simplify ctrl flow graph by, for example, deleting unreachable code
	llvm::FunctionPass* pass1 = llvm::createPromoteMemoryToRegisterPass();	// heavily reduce stack traffic, since everything is now a load or a store
	llvm::FunctionPass* pass2 = llvm::createInstructionCombiningPass();		// ???
	llvm::FunctionPass* pass3 = llvm::createReassociatePass();				// self explanatory i guess
	legacy_fn_pass_mgr->add(pass1);
	legacy_fn_pass_mgr->add(pass2);
	legacy_fn_pass_mgr->add(pass3);

	// Register analysis passes that are used by these transform passes
	pass_builder->registerModuleAnalyses(*module_analysis_mgr);
	pass_builder->registerCGSCCAnalyses(*cgscc_analysis_mgr);
	pass_builder->registerFunctionAnalyses(*fn_analysis_mgr);
	pass_builder->registerLoopAnalyses(*loop_analysis_mgr);
	pass_builder->crossRegisterProxies(*loop_analysis_mgr, *fn_analysis_mgr, *cgscc_analysis_mgr, *module_analysis_mgr);

	// Add the prelude - Deprecated, this is done in main:main()
	// add_prelude();
}

// for example, generate_llvm_declaration("print", "void", 1, __Pointer)
void IRGenerator::generate_llvm_declaration(const std::string& function_name, const std::string& return_type, int argument_count, ...) {
	IRGenerator* gen = this;

	va_list args;
	std::vector<llvm::Type*> vec;

	std::vector<const char*> strings;
	char* current_str = "invalidtype";

	va_start(args, argument_count);
	for (int i = 0; i < argument_count; i++) {
		current_str = va_arg(args, char*);
		strings.push_back(current_str);
	}
	va_end(args);


	for (int i = 0; i < argument_count; i++) {
		std::string current_arg_type = strings[i];
		const salt::Type* salt_ty = salt::all_types[current_arg_type];

		if (!salt_ty)
			salt::print_fatal(salt::f_string("Bad salt type in add_prelude: %s", current_arg_type.c_str()));
		
		llvm::Type* ty = const_cast<llvm::Type*>(salt_ty->get());
		
		if (!ty)
			salt::print_fatal(salt::f_string("Bad LLVM type in add_prelude: %s", current_arg_type.c_str()));

		vec.push_back(ty);
	}

	llvm::ArrayRef vec_ref = vec;

	const salt::Type* attempted_return_type = salt::all_types[return_type];
	if (!attempted_return_type)
		salt::print_fatal(salt::f_string("Bad salt return type in add_prelude: %s", return_type.c_str()));

	llvm::Type* attempted_llvm_ty = const_cast<llvm::Type*>(attempted_return_type->get());
	if (!attempted_llvm_ty)
		salt::print_fatal(salt::f_string("Bad LLVM return type in add_prelude: %s", return_type.c_str()));



	llvm::FunctionType* ft = llvm::FunctionType::get(attempted_llvm_ty, vec_ref, false);
	llvm::Function::Create(ft, llvm::Function::ExternalLinkage, function_name, *gen->mod.get());
}

// Add the std parts of the prelude, for example print and scan functions.
void IRGenerator::add_std_prelude() {
	generate_llvm_declaration("print", "void", 1, "__Pointer");
	generate_llvm_declaration("scan", "void", 2, "__Pointer", "usize");
}

void IRGenerator::add_prelude() {
	if (!salt::no_std)
		add_std_prelude();
}

llvm::AllocaInst*& IRGenerator::find_in_named_values(const std::string& variable_name) {
	salt::dboutv << "Finding " << variable_name << " in named values\n";

	int current_scope = this->named_values.size() - 1;
	ASSERT(this->named_values.size() > 0);
	llvm::AllocaInst* current_match = nullptr;


	while (current_scope >= 0 && current_match == nullptr) {
		current_match = this->named_values[current_scope][variable_name];
		current_scope--;
		if (current_match != nullptr)
			current_scope++; // un-increment the scope, since we found the variable we're looking for
	}

	if (!current_match) // did not find this variable
		current_scope = this->named_values.size() - 1; // reset the scope to the innermost, so that we can create a poison variable at not-global level

	// we create this volatile variable to be sure there's an AllocaInst* 
	// at named_values[current_scope][variable_name], even if that AllocaInst* is nullptr

	

	llvm::AllocaInst* anti_crasher = this->named_values[current_scope][variable_name];

	if (anti_crasher)
		salt::dboutv << variable_name << " found in named values at scope " << current_scope << '\n';
	else
		salt::dboutv << variable_name << " not found in named values!\n";

	return this->named_values[current_scope][variable_name];
}



IRGeneratorException::IRGeneratorException(int line, int col, const char* str) :
	Exception(std::to_string(line) + ':' + std::to_string(col) + ": " + str) {}

IRGeneratorException::IRGeneratorException(int line, int col, const std::string& str) :
	Exception(std::to_string(line) + ':' + std::to_string(col) + ": " + str) {}