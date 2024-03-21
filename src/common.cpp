#include "common.h"
#include <limits.h>
#include <cstdarg>

using namespace salt;

// Global variables
bool any_compile_error_occured = false;
NullStream salt::null_stream = NullStream();
std::ostream& salt::dbout = SALT_INTERNAL_DBOUT;
std::ostream& salt::dberr = SALT_INTERNAL_DBERR;


unsigned long long salt::atoull(const char* s) {
	if (*s == '\0')
		throw Exception("Tried to parse empty string with salt::atoull");
	if (*s == '.')
		throw Exception("Invalid string for salt::atoull: .");

	unsigned long long res = 0;

	while (*s) {
		if (*s == '.')
			break;

		if (*s < '0' || *s > '9')
			throw Exception(std::string("Invalid string for salt::atoull: ") + s);

		unsigned long long next_res = res * 10 + *s - '0';

		if (next_res < res)
			throw Exception(std::string("Overflow in salt::atoull for number: ") + s);

		res = next_res;
		s++;
	}
	return res;
}



long long salt::atoll(const char* s) {
	if (s == std::to_string(LLONG_MIN))
		return LLONG_MIN;

	bool neg = s[0] == '-';
	if (neg)
		s++;

	// I don't know of any other portable way to handle overflow without UB
	// So in this fn we check if overflow happened by checking if res wrapped around
	// That's defined behavior for unsigned ints but UB for signed ints
	
	// (don't mind the cursed "const char* - bool")
	unsigned long long res = salt::atoull(s);
	if (res > LLONG_MAX)
		throw Exception(std::string("Overflow in salt::atoll for number: ") + (s - neg));


	return neg ? -res : res;
}

int salt::atoi(const char* s) {
	long long res = salt::atoll(s);
	if (res > INT_MAX || res < INT_MIN)
		throw Exception(std::string("Overflow in salt::atoi for number: ") + s);
	return static_cast<int>(res);
}

long long salt::llrand() {
	return (static_cast<long long>(std::rand()) << 32) | std::rand();
}


std::string salt::reverse(const std::string& s) {
	std::string res;
	for (int i = s.size() - 1; i >= 0; i--)
		res.push_back(s[i]);
	return res;
}

// class TextColor
#ifdef SALT_WINDOWS
std::ostream& salt::operator<<(std::ostream& os, const TextColor tc) {
	TextColor::set(tc);
	return os;
}

void TextColor::set(const TextColor tc) {
	using namespace Windows;
	Windows::HANDLE console_handle = Windows::GetStdHandle(STD_OUTPUT_HANDLE);
	Windows::SetConsoleTextAttribute(console_handle, tc.color_);
}
#endif


// Formatted std::string
std::string salt::f_string(const char* format, ...) {
	std::string str;
	// Initialize a variable argument list
	va_list args;
	va_start(args, format);

	// Determine the length of the formatted string
	// by calling std::vsnprintf with NULL buffer
	int length = std::vsnprintf(nullptr, 0, format, args);

	// Resize the string to accommodate the formatted output
	// plus one to accomodate the null character of course
	str.resize(length + 1);

	// Format the string
	// once again, plus one to print the last character
	std::vsnprintf(&str[0], length + 1, format, args);

	// Clean up the variable argument list
	va_end(args);
	return str;
}