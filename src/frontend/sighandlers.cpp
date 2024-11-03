#include "sighandlers.h"

// Windows
#if SALT_WINDOWS
Windows::LONG WINAPI exception_handler(Windows::EXCEPTION_POINTERS* exception_info) {
	using namespace Windows;
	char print_buf[512] = {};
	switch (exception_info->ExceptionRecord->ExceptionCode) {

    // thyank you chatgpt
    case EXCEPTION_ACCESS_VIOLATION: {
        // ExceptionInformation[0] == 0 for a read violation, 1 for a write violation
        const char* violation_type = exception_info->ExceptionRecord->ExceptionInformation[0] == 0
            ? "read"
            : "write";
        snprintf(print_buf, 511, "Access violation (%s) at address 0x%p",
            violation_type,
            (void*)exception_info->ExceptionRecord->ExceptionInformation[1]);
        break;
    }
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        snprintf(print_buf, 511, "Array bounds exceeded exception");
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        snprintf(print_buf, 511, "Datatype misalignment exception");
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        snprintf(print_buf, 511, "Floating-point divide by zero exception");
        break;
    case EXCEPTION_FLT_OVERFLOW:
        snprintf(print_buf, 511, "Floating-point overflow exception");
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        snprintf(print_buf, 511, "Floating-point underflow exception");
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        snprintf(print_buf, 511, "Integer divide by zero exception");
        break;
    case EXCEPTION_INT_OVERFLOW:
        snprintf(print_buf, 511, "Integer overflow exception");
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        snprintf(print_buf, 511, "Illegal instruction exception");
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        snprintf(print_buf, 511, "Page fault exception at address 0x%p with NTSTATUS code 0x%08X",
            (void*)exception_info->ExceptionRecord->ExceptionInformation[1],
            (unsigned int)exception_info->ExceptionRecord->ExceptionInformation[2]);
        break;
    case EXCEPTION_STACK_OVERFLOW:
        snprintf(print_buf, 511, "Stack overflow exception");
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        snprintf(print_buf, 511, "Privileged instruction exception");
        break;
    default:
        snprintf(print_buf, 511, "Unhandled Windows exception: code 0x%x", exception_info->ExceptionRecord->ExceptionCode);
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