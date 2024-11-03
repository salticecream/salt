#include "sighandlers.h"

// Windows
#if SALT_WINDOWS
Windows::LONG WINAPI exception_handler(Windows::EXCEPTION_POINTERS* exception_info) {
	using namespace Windows;
	char print_buf[512] = {};
	switch (exception_info->ExceptionRecord->ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:
		snprintf(print_buf, 511, "access violation at address 0x%p", exception_info->ExceptionRecord->ExceptionInformation[1]);
		break;
	default:
		snprintf(print_buf, 511, "unknown unhandled Windows exception");
		break;
	}
	salt::print_fatal(print_buf, exception_info->ExceptionRecord->ExceptionCode);
}

void register_signal_handlers() {
	Windows::SetUnhandledExceptionFilter(&exception_handler);
}

// Linux
#elif SALT_LINUX
static_assert(0, "salt is only supported on Windows!");


#else
static_assert(0, "salt is only supported on Windows!");

#endif