#pragma once
#include "../common.h"
#include <csignal>

void register_signal_handlers();

// Windows
#if SALT_WINDOWS

// Linux
#elif SALT_LINUX
static_assert(0, "salt is only supported on Windows!");


#else
static_assert(0, "salt is only supported on Windows!");

#endif