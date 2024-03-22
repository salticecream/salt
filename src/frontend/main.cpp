#include <iostream>
#include "lexer.h"
#include "parser.h"
#include "../common.h"
#include "irgenerator.h"
#include "flags.h"

#ifdef NDEBUG
#define ASTCNDEBUG 1
#endif

// PLACEHOLDERS
typedef int option_t;
std::vector<option_t> placeholder_vector_dontuse = {};
static int files_compiled = 0;

// Only for windows, only to .o
static void compile_to_object(const std::vector<CompilerFlag> /*compiler_flags*/) {
    using namespace salt;
    std::string target_triple = llvm::sys::getDefaultTargetTriple();
    salt::dbout << "target triple: " << target_triple;
    std::string error;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        std::cerr << "you fucked up: " << error;
        std::exit(1);
    }

    const char* cpu_type = "generic";
    const char* features = "";
    llvm::TargetOptions opt;
    llvm::TargetMachine* target_machine = target->createTargetMachine(target_triple, cpu_type, features, opt, llvm::Reloc::PIC_);
    
    IRGenerator* gen = IRGenerator::get();
    gen->mod->setDataLayout(target_machine->createDataLayout());
    gen->mod->setTargetTriple(target_triple);

    std::string output_file = "d:/work2/salt/saltc-out/output";
    output_file += std::to_string(++files_compiled);
    output_file += ".o";

    std::error_code error_code;
    llvm::raw_fd_ostream destination(output_file.c_str(), error_code, llvm::sys::fs::OpenFlags::OF_None);

    if (error_code) {
        std::cerr << "could not open file: " << error_code.message();
        std::exit(1);
    }

    // finally, emit this object code
    llvm::legacy::PassManager pass;

    auto file_type = llvm::CodeGenFileType::ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, destination, nullptr, file_type)) {
        std::cerr << "Could not emit file of this type";
        std::exit(1);
    }

    pass.run(*gen->mod);
    destination.flush();
    
}

static void set_flags(const std::vector<CompilerFlag> flags) {
    
    for (const CompilerFlag compiler_flag : flags) {
        const Flags_e& flag = compiler_flag.flag;

        using f = Flags_e;
        switch (flag) {
        case f::DEBUG_OUTPUT_VERBOSE:
            salt::dboutv.activate();
            salt::dberrv.activate();
        case f::DEBUG_OUTPUT:
            salt::dbout.activate();
            salt::dberr.activate();
            break;
        default:
            throw std::exception("bad flag to set_flags()");
        }
    }
}


int main(int argc, const char** argv) {
    if (argc <= 1) {
        std::cerr << "error: no input files";
        exit(1);
    }

    std::vector<const char*> input_files;
    std::vector<CompilerFlag> compiler_flags;
        
    // Read argv and populate input_files and compiler_flags
    // If argc == 0 then continue as usual (but this is deprecated)
    for (int i = 1; i < argc; i++) {
        if (Flags::all_flags.count(argv[i])) /* if argv[i] is a flag, then */ {
            // add this flag to compiler_flags.
            /// @todo: add flags which are options/have data (for example: --optimizations=O3)
            compiler_flags.push_back(CompilerFlag(Flags::all_flags[argv[i]]));

        } else if (string_ends_with(argv[i], ".sl")) {
            input_files.push_back(argv[i]);
        } else {
            std::cerr << salt::f_string("error: could not parse file or flag \"%s\"", argv[i]);
            exit(1);
        }
    }

        

    try {
        set_flags(compiler_flags);
        MiniRegex::fill_types();
        BinaryOperator::fill_map();
        
        std::vector<Token> vec;

        for (const char* input_file : input_files) {
            // Tokenize the file
            Lexer* lexer = Lexer::get();
            vec = lexer->tokenize(input_file);
            salt::dbout << "Done tokenizing" << std::endl;
            for (const salt::Exception& e : lexer->errors())
                salt::dbout << e.what() << std::endl;
            
            // Parse tokens and form AST
            Parser* parser = Parser::get(vec);
            parser->parse();

            // Compile the file
            compile_to_object(compiler_flags);

            // So we can start compiling the next file
            Lexer::destroy();
            Parser::destroy();
            IRGenerator::destroy();
        }
        return 0;
        
    // Exception handling
    } catch (const std::exception& e) {
        std::cerr << "\n__________________________________\n";
        std::cerr << "--- " << salt::TextColor(FOREGROUND_RED | FOREGROUND_INTENSITY) << "FATAL: unhandled exception " << salt::TextColor(0x07) << "---\n" << e.what() << std::endl;
        return -20;

    } catch (...) {
        std::cerr << "fatal: unknown exception" << std::endl;
        return -30;
    }
}



/*
while (1) {
        try {
            main_loop();
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
}


void main_loop() {
    Lexer* lexer = Lexer::get();
    while (1) {
        std::string line;
        std::getline(std::cin, line);
        std::vector<Token> vec = tokenize(line.c_str());
        Parser* parser = Parser::get(vec);
    }
}
*/