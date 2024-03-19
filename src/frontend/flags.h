#pragma once

#include <cstring>
namespace Flags 
{

enum Flags_e {
    NO_NEWBIE_WARNINGS = 0,
    DONT_BUILD,
    TOTAL,
};

extern bool flags[Flags::TOTAL];

// Reset the compiler flags
void reset();

// Set compiler flags using command-line arguments.
// Returns the index of the first bad argument (starting at 1), or 0 for success.
int set(int argc, const char** argv);

}