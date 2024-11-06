#pragma once
#include "../common.h"
#include "frontendllvm.h"
#include "tokens.h"
#include <map>

/*
* Defines the IRGenerator class, which generates and optimizes LLVM IR.
* If you were looking for the IR generation code, go to ast.cpp
*
* Due to reasons, the context is outside of the IRGenerator but should still be referred to using the IRGenerator
*/

extern std::unique_ptr<llvm::LLVMContext> global_context;

class IRGenerator {
private:
	static IRGenerator* instance;
	void add_prelude();
	void add_std_prelude();
	void generate_llvm_declaration(const std::string& function_name, const std::string& return_type, int argument_count, ...);
	IRGenerator();

public:
	// The context is an opaque object that I don't understand, that won't be necessary to understand according to the tutorial
	std::unique_ptr<llvm::LLVMContext>& context;

	// The builder is a helper object that makes it easy to generate LLVM instructions
	std::unique_ptr<llvm::IRBuilder<>> builder;

	// The module is an object that contains all of the memory for the IR that is generated
	// When the module is created, we will also declare the intrinsic functions that form the prelude
	std::unique_ptr<llvm::Module> mod;

	// Keeps track of all named values.
	/// @todo: add scope (std::map of (Scope and std::map of std::string and llvm::Value*)) or similar)
	std::map<std::string, llvm::AllocaInst*> named_values;

	std::map<std::string, llvm::Constant*> named_strings;

	std::map<std::string, llvm::Function*> named_functions;


	// For optimization purposes
	std::unique_ptr<llvm::LoopAnalysisManager> loop_analysis_mgr;
	std::unique_ptr<llvm::FunctionPassManager> fn_pass_mgr;
	std::unique_ptr<llvm::legacy::FunctionPassManager> legacy_fn_pass_mgr;
	std::unique_ptr<llvm::FunctionAnalysisManager> fn_analysis_mgr;
	std::unique_ptr<llvm::CGSCCAnalysisManager> cgscc_analysis_mgr; // call graph strongly connected component
	std::unique_ptr<llvm::ModuleAnalysisManager> module_analysis_mgr;
	std::unique_ptr<llvm::PassInstrumentationCallbacks> pass_instrumentation_callbacks;
	std::unique_ptr<llvm::StandardInstrumentations> std_instrumentations;

	llvm::PassBuilder pass_builder;



	static IRGenerator* get();
	static void destroy();

};


class IRGeneratorException : public salt::Exception {
public:
	IRGeneratorException(int line, int col, const char* s);
	IRGeneratorException(int line, int col, const std::string& s);
};