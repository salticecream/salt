#include <iostream>
#include "lexer.h"
#include "parser.h"
#include "../common.h"
#include "irgenerator.h"

// preprocessor trickery
#ifdef NDEBUG
#define ASTCNDEBUG 1
#endif

// PLACEHOLDER
typedef int option_t;

// Only for windows, only to .o
static void compile(uint64_t flags = 0, std::vector<option_t>&& = std::vector<option_t>()) {
    using namespace salt;
    std::string target_triple = llvm::sys::getDefaultTargetTriple();
    std::cout << "target triple: " << target_triple;
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

    const char* output_file = "d:/work2/salt/saltc-out/output.o";
    std::error_code error_code;
    llvm::raw_fd_ostream destination(output_file, error_code, llvm::sys::fs::OpenFlags::OF_None);

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

// this will only spit out the tokens (of the input file) for the time being.
int main(int argc, const char * const * const argv) {
    
    #ifndef NDEBUG
        std::cerr << "note: debugging is on (NDEBUG not defined)" << std::endl;
#endif

    // Real main function
    try {


        auto irg = IRGenerator::get();
        MiniRegex::fill_types();
        BinaryOperator::fill_map();

        std::vector<Token> vec = tokenize(argv[1]);
        std::cout << "Done tokenizing" << std::endl;

        Parser* parser = Parser::get(vec);
        std::cout << "Done getting the parser" << std::endl;

        parser->parse();
        compile();
        




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