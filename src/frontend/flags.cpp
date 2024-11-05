#include "flags.h"

CompilerFlag::CompilerFlag(Flags_e f_flag) : flag(f_flag) {}
CompilerFlag::CompilerFlag(Flags_e f_flag, const std::string& f_data) : flag(f_flag), data(f_data) {}

namespace Flags {
	std::map<std::string, Flags_e> Flags::all_flags = {
		{"--dbo", Flags_e::DEBUG_OUTPUT},				// debug output
		{"--dbv", Flags_e::DEBUG_OUTPUT_VERBOSE},		// debug output (verbose)
		{"--nostd", Flags_e::NO_STD},					// doesn't link to any library like libc/kernel32.dll, only core + prelude
	};
}

