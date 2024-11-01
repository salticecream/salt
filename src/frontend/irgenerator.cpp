#include "irgenerator.h"
#include <iostream>
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
	this->named_values = std::map<std::string, llvm::Value*>();
	this->named_strings = {};


	// For optimization and whatnot
	loop_analysis_mgr = std::make_unique<llvm::LoopAnalysisManager>();
	fn_pass_mgr = std::make_unique<llvm::FunctionPassManager>();
	fn_analysis_mgr = std::make_unique<llvm::FunctionAnalysisManager>();
	cgscc_analysis_mgr = std::make_unique<llvm::CGSCCAnalysisManager>();
	module_analysis_mgr = std::make_unique<llvm::ModuleAnalysisManager>();
	pass_instrumentation_callbacks = std::make_unique<llvm::PassInstrumentationCallbacks>();
	std_instrumentations = std::make_unique<llvm::StandardInstrumentations>(*context, /*DebugLogging = */ true);
	std_instrumentations->registerCallbacks(*pass_instrumentation_callbacks, module_analysis_mgr.get());

	// Add optimization passes
	fn_pass_mgr->addPass(llvm::InstCombinePass());	// optimize calculations
	fn_pass_mgr->addPass(llvm::ReassociatePass());	// reassociate expressions
	fn_pass_mgr->addPass(llvm::GVNPass());			// eliminate common subexpressions so they only need to be calculated once
	fn_pass_mgr->addPass(llvm::SimplifyCFGPass());	// simplify ctrl flow graph by, for example, deleting unreachable code

	// Register analysis passes that are used by these transform passes
	pass_builder.registerModuleAnalyses(*module_analysis_mgr);
	pass_builder.registerCGSCCAnalyses(*cgscc_analysis_mgr);
	pass_builder.registerFunctionAnalyses(*fn_analysis_mgr);
	pass_builder.registerLoopAnalyses(*loop_analysis_mgr);
	pass_builder.crossRegisterProxies(*loop_analysis_mgr, *fn_analysis_mgr, *cgscc_analysis_mgr, *module_analysis_mgr);
}

IRGeneratorException::IRGeneratorException(int line, int col, const char* str) :
	Exception(std::to_string(line) + ':' + std::to_string(col) + ": " + str) {}

IRGeneratorException::IRGeneratorException(int line, int col, const std::string& str) :
	Exception(std::to_string(line) + ':' + std::to_string(col) + ": " + str) {}