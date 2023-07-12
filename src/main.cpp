#include <cstring>
#include <cstdio>
#include "flags.h"
#include "test_header.h"
#include "lexer.h"

int main(int argc, const char** argv) {
    // set compiler flags using command-line args
    test_function();

    Flags::reset();
    int flag_failure = Flags::set(argc, argv);

    if (flag_failure) {
        printf("fatal error: bad arg %s\n", argv[flag_failure]);
        return flag_failure;
    }
    
    

    return 0;
}



