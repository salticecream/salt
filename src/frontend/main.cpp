#include <iostream>
#include <cstdlib>
#include "lexer.h"
#include "parser.h"
#include "../common.h"
#include "irgenerator.h"
#include "flags.h"
#include "sighandlers.h"

#ifdef NDEBUG
#define ASTCNDEBUG 1
#endif

bool salt::no_std = false; // common.h
static int files_compiled = 0;
static bool any_compile_error_in_any_file = false;
static const char* libraries = "kernel32.lib user32.lib msvcrt.lib";
static std::vector<std::string> compiled_files; // we will link them all together
static int link_all();
std::string output_name = "a";
static bool user_chosen_output_name = false;
const char* PRELUDE_FILE = "prelude.sl";
static llvm::OptimizationLevel optimization_level = llvm::OptimizationLevel::O0;
std::vector<std::string> salt::file_names = { PRELUDE_FILE };
int salt::current_file_name_index = 0;

// Only for windows, only to .o
static void compile_to_object(const std::vector<CompilerFlag>& /*compiler_flags*/) {
    using namespace salt;
    std::string target_triple = llvm::sys::getDefaultTargetTriple();
    salt::dbout << "target triple: " << target_triple << '\n';
    std::string error;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmParser();
    llvm::InitializeNativeTargetAsmPrinter();

    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, error);

    if (!target) {
        print_fatal(error);
    }

    const char* cpu_type = "generic";
    const char* features = "";
    llvm::TargetOptions opt{};
    llvm::TargetMachine* target_machine = target->createTargetMachine(target_triple, cpu_type, features, opt, llvm::Reloc::PIC_);
    
    IRGenerator* gen = IRGenerator::get();
    gen->mod->setDataLayout(target_machine->createDataLayout());
    gen->mod->setTargetTriple(target_triple);
    gen->legacy_fn_pass_mgr->doInitialization();

    // optimishimishimizations

    // for (auto& func : gen->mod->functions())
    //     gen->fn_pass_mgr->run(func, *gen->fn_analysis_mgr);

    // for (auto& func : gen->mod->functions())
    //    gen->legacy_fn_pass_mgr->run(func);

    if (optimization_level != llvm::OptimizationLevel::O0) {
        llvm::ModulePassManager module_pass_mgr = gen->pass_builder->buildPerModuleDefaultPipeline(optimization_level);
        module_pass_mgr.run(*gen->mod, *gen->module_analysis_mgr);
    }


    std::string output_file = "__SaltOutputObjectTmp";
    output_file += std::to_string(++files_compiled);
    output_file += ".o";
    compiled_files.push_back(output_file);

    std::error_code error_code;
    llvm::raw_fd_ostream destination(output_file.c_str(), error_code, llvm::sys::fs::OpenFlags::OF_None);

    if (error_code) {
        print_fatal("could not open file: " + error_code.message());
    }

    // finally, emit this object code
    llvm::legacy::PassManager pass;

    auto file_type = llvm::CodeGenFileType::ObjectFile;

    if (target_machine->addPassesToEmitFile(pass, destination, nullptr, file_type)) {
        print_fatal("could not emit file of this type");
    }

    pass.run(*gen->mod);
    destination.flush();
    
}

static void set_flags(const std::vector<CompilerFlag>& flags) {

    for (const CompilerFlag compiler_flag : flags) {
        const Flags_e& flag = compiler_flag.flag;

        using f = Flags_e;
        switch (flag) {
        case f::DEBUG_OUTPUT_VERBOSE:
            salt::dboutv.activate();
            salt::dberrv.activate();
        // intentional fallthrough
        case f::DEBUG_OUTPUT:
            salt::dbout.activate();
            salt::dberr.activate();
            break;
        case f::NO_STD:
            salt::no_std = true;
            break;
        default:
            salt::print_fatal(salt::f_string("bad flag to set_flags(): %d", flag));
        }
    }

    salt::dboutv << "Flags set\n";
}


int main(int argc, const char** argv) {
    register_signal_handlers();
    std::vector<const char*> input_files;
    std::vector<CompilerFlag> compiler_flags;
        
    // Read argv and populate input_files and compiler_flags
    // If argc == 0 then continue as usual (but this is deprecated)
    for (int i = 1; i < argc; i++) {

        // Handle -o so the user can choose which file to output to
        if (argv[i] == std::string("-o")) {
            i++;
            if (i < argc && argv[i]) { // redundancy
                output_name = argv[i];
                user_chosen_output_name = true;
                continue;
            } else {
                salt::print_fatal("expected file name after -o");
            }
        }

        if (Flags::all_flags.count(argv[i])) /* if argv[i] is a flag, then */ {
            // add this flag to compiler_flags.
            /// @todo: add flags which are options/have data (for example: -o output.exe)
            compiler_flags.push_back(CompilerFlag(Flags::all_flags[argv[i]]));


        } else if (string_ends_with(argv[i], ".sl")) {
            input_files.push_back(argv[i]);

        } else {
            salt::print_fatal(salt::f_string("could not parse file or flag \"%s\"", argv[i]));
        }
    }

    if (input_files.empty())
        salt::print_fatal("no input files");

    set_flags(compiler_flags);
    BinaryOperator::fill_map();
    salt::fill_types();

    try {
        int next_file_name_index = 0;
        for (const char* input_file : input_files) {
            next_file_name_index++;
            salt::file_names.push_back(input_file);
            // read the prelude, which consists of function headers
            any_compile_error_occured = false;
            salt::current_file_name_index = 0;
            Lexer* lexer = Lexer::get();
            std::vector<Token> vec = lexer->tokenize(PRELUDE_FILE);
            while (vec.size() && vec.back().val() == TOK_EOF)
                vec.pop_back();
            vec.push_back(TOK_EOL); vec.push_back(TOK_EOL);
            Lexer::destroy();

            // read the current file
            salt::current_file_name_index = next_file_name_index;
            lexer = Lexer::get();
            std::vector<Token> input_vec = lexer->tokenize(input_file);
            vec.insert(vec.end(), std::make_move_iterator(input_vec.begin()), std::make_move_iterator(input_vec.end()));
            salt::dbout << "Done tokenizing" << std::endl;
            for (const salt::Exception& e : lexer->errors())
                salt::dbout << e.what() << std::endl;
            
            // Parse tokens and form AST
            Parser* parser = Parser::get(vec);
            parser->parse();
            

            // Compile the file
            if (!any_compile_error_occured)
                compile_to_object(compiler_flags);
            else
                any_compile_error_in_any_file = true;

            // Reset everything so we can start compiling the next file
            Lexer::destroy();
            Parser::destroy();
            IRGenerator::destroy();
        }
        if (!any_compile_error_in_any_file && salt::main_function_found)
            salt::dbout << salt::Color::GREEN << "\nCompilation success!\n" << salt::Color::WHITE;

        if (!salt::main_function_found)
            salt::print_fatal("no main function found");

        int main_res = EXIT_FAILURE;
        if (!any_compile_error_in_any_file)
            main_res = link_all();

        return main_res;
        
    // Exception handling
    } catch (const std::exception& e) {
        std::cerr << "\n__________________________________\n";
        std::cerr << "--- " << salt::Color::LIGHT_RED << "FATAL: unhandled exception " << salt::Color::WHITE << "---\n" << e.what() << std::endl;
        return EXIT_FAILURE;

    } catch (...) {
        std::cerr << "fatal: unknown exception" << std::endl;
        return EXIT_FAILURE;
    }

}


static int link_all() {
    if (salt::no_std && !user_chosen_output_name)
        output_name += ".bin";
    else if (!salt::no_std && !user_chosen_output_name)
        output_name += ".exe";

    // safe? should be safe because file names inputted here can't contain escape symbols
    std::string command_to_run = "lld-link /subsystem:console /out:" + output_name + ' '; 

    for (const std::string& file_name : compiled_files)
        command_to_run += file_name + ' ';

    if (!salt::no_std)
        command_to_run += libraries;
    else
        command_to_run += "/nodefaultlib ";

    salt::dboutv << "Linker command: " << command_to_run << '\n';
    int res = std::system(command_to_run.c_str());

    //remember to clean up by removing the tmp files we made
    for (const std::string& file_name : compiled_files)
        std::system(salt::f_string("rm %s", file_name.c_str()).c_str());

    return res;

}