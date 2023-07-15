/* 
* This file is part of the salt project, which can be found on GitHub here:
* https://github.com/salticecream/salt
*
* The license for this project, including this file, is the GNU GPL v3 license.
*/

/*
* My specialized build tool for salt.
* This file requires C++17 or higher to compile, and the resulting executable
* uses Clang to compile the other source files.
*/

#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

namespace fs = std::filesystem;
using std::string;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;

static std::vector<string> files;
static const string USAGE = "usage: smake [path|-s] [-s]";

// Find the necessary .cpp files and add them to the "files" vector.
// Exits on error.
static void add_files(fs::path dir);

// Print the elements of the "files" vector.
// No error code, because this shouldn't ever fail.
static void print_vector(bool verbose);

// Finally compiles the files of the "files" vector.
static void compile_files(string dir, bool make_tests);

int main(int argc, char** argv) {
    auto start_time = high_resolution_clock::now();
    // generate the regular files
    bool verbose = true;
    string dir = "..";

    if (argc == 2)
        if (!strcmp("-s", argv[1]))
            verbose = false;
        else
            dir = argv[1];
    
    else if (argc == 3) {
        dir = argv[1];
        if (!strcmp("-s", argv[2]))
            verbose = false;
        else
            std::cout << "fatal error: syntax error\n" << USAGE << std::endl;
    }
    else if (argc > 3) {
        std::cout << "fatal error: syntax error\n" << USAGE << std::endl;
        return argc;
    }


    add_files(fs::path(dir));    
    print_vector(verbose);

    if (!files.empty()) {
        compile_files(string(dir), false);
        /* return 0 */
    } else {
        std::cout << "fatal error: no files found" << std::endl;
        return -1;
    }

    auto end_time = high_resolution_clock::now();
    auto time_diff_ms = duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << std::endl << std::endl << 
        "smake took (" << time_diff_ms << " ms)" << std::endl;
    return 0;
}

// Note: dir should always be "..", cba to change this
static void compile_files(string dir, bool make_tests) {
    string command = "clang ";
    for (const auto file : files)
        command += " " + file;
    
    command += " -std=c++17 -o ";
    command += dir;
    command += '/';
    if (make_tests)
        command += "tests/all_tests.exe";
    else
        command += "bin/main.exe";

    std::system(command.c_str());
}


static void add_files(fs::path dir) {
try {
    for (const auto& elem : fs::directory_iterator(dir)) {
        const auto& path = elem.path();

        if (path.stem().string() == "smake" || path.filename().string() == "smake.cpp") {
            std::cout << "skipped: " << path << std::endl;
            continue;
        }
        
        if (fs::is_directory(path))
            add_files(path);
        
        else if (path.extension() == ".cpp")
            files.push_back(path.string());

    }
} catch (const fs::filesystem_error& e) {
        if (e.code() == std::errc::no_such_file_or_directory) {
            std::cout << "fatal: directory " << dir 
                << " not found" << std::endl;
            exit(e.code().value());
        } else {
            std::cout << "fatal: c++ exception: " << e.what() << std::endl;
            exit(e.code().value());
        }
} catch (const std::exception& e) {
        std::cout << "fatal: c++ exception:\n" << e.what() << std::endl;
        exit(1);
    }
}


static void print_vector(bool verbose) {
    std::cout << "\ncompiling " << files.size() <<
        " files";
    if (verbose) {
        std::cout << ':' << std::endl;
        for (string s : files)
            std::cout << s << std::endl;
    }
    
    std::cout << std::endl;
}

