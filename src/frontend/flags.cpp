#include "flags.h"

namespace Flags 
{

bool flags[TOTAL];

// Reset the compiler flags
void reset() {
    for (int i = 0; i < Flags::TOTAL; i++)
        flags[i] = false;
}

// Set compiler flags using command-line arguments.
// Returns the index of the first bad argument (starting at 1), or 0 for success.
int set(int argc, const char** argv) {
    for (int i = 1; i < argc; i++)

        // NO_NEWBIE_WARNINGS
        if (!strcmp(argv[i], "-nnw"))
            flags[Flags::NO_NEWBIE_WARNINGS] = true;

        // DONT_BUILD
        else if (!strcmp(argv[i], "-dontbuild"))
            flags[Flags::DONT_BUILD] = true;

        // (error)
        else
            return i;
    
    return 0;
}

}
