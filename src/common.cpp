#include "common.h"
#include <limits.h>
#include <cstdarg>

using namespace salt;

// Global variables
bool any_compile_error_occured = false;
MaybeStream salt::dbout;
MaybeStream salt::dberr;
MaybeStream salt::dboutv;
MaybeStream salt::dberrv;

void salt::MaybeStream::activate() {
	print_to_stdout_ = true;
}

void salt::MaybeStream::deactivate() {
	print_to_stdout_ = false;
}

bool salt::MaybeStream::is_active() const {
	return print_to_stdout_;
}

unsigned long long salt::atoull(const char* s) {
	if (*s == '\0')
		print_fatal("Tried to parse empty string with salt::atoull");
	if (*s == '.')
		print_fatal("Invalid string for salt::atoull: .");

	unsigned long long res = 0;

	while (*s) {
		if (*s == '.')
			break;

		if (*s < '0' || *s > '9')
			print_fatal(std::string("Invalid string for salt::atoull: ") + s);

		unsigned long long next_res = res * 10 + *s - '0';

		if (next_res < res)
			print_fatal(std::string("Overflow in salt::atoull for number: ") + s);

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
		print_fatal(std::string("Overflow in salt::atoll for number: ") + (s - neg));


	return neg ? -res : res;
}

int salt::atoi(const char* s) {
	long long res = salt::atoll(s);
	if (res > INT_MAX || res < INT_MIN)
		print_fatal(std::string("Overflow in salt::atoi for number: ") + s);
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
	if (Windows::HANDLE console_handle = Windows::GetStdHandle(STD_OUTPUT_HANDLE))
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

void salt::print_colored(const std::string& str, const TextColor tc) {
	std::cout << tc << str << Color::WHITE;
}

void salt::print_warning(const std::string& str) {
	print_colored("warning: " + str, Color::YELLOW);
	std::cout << std::endl;
}

void salt::print_error(const std::string& str) {
	any_compile_error_occured = true;
	print_colored("error: " + str, Color::LIGHT_RED);
	std::cout << std::endl;
}

[[noreturn]]
void salt::print_fatal(const std::string& str, int exit_code) {
	any_compile_error_occured = true;
	print_colored("fatal: " + str, Color::LIGHT_RED);
	std::cout << std::endl;
	std::exit(exit_code);
}