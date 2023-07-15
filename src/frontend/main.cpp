#include <cstring>
#include <iostream>
#include <filesystem>
#include "lexer.h"

namespace fs = std::filesystem;

// this will only spit out the tokens (of the input file) for the time being.
int main(int argc, const char** argv) {
    MiniRegex::fill_types();
    auto vec = std::vector<Token>();
    int res = tokenize(argv[1], vec);
    std::cout << std::endl << "done (" << res << " tokens)" << std::endl;

    
    return 0;
}



