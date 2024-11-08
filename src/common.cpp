#include "common.h"
#include <limits.h>
#include <cstdarg>
#include "frontend/miniregex.h"
#include "frontend/tokens.h"
#include "frontend/ast.h"

int salt::WARNING_LEVEL = 255;
using namespace salt;

// Global variables
bool any_compile_error_occured = false;
bool salt::main_function_found = false;
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

void salt::print_warning(const std::string& str, int min_warning_level) {
	if (min_warning_level > salt::WARNING_LEVEL)
		return;

	print_colored("warning: ", Color::YELLOW);
	std::cout << str << std::endl;
}

void salt::print_warning_at(const Token& tok, const std::string& str, int min_warning_level) {
	if (min_warning_level > salt::WARNING_LEVEL)
		return;

	print_colored("warning: ", Color::YELLOW);
	std::cout << salt::file_names[current_file_name_index] << ':' << tok.line() << ':' << tok.col() << ": " << str << std::endl;
}

void salt::print_warning_at(ExprAST* expr, const std::string& str, int min_warning_level) {
	if (min_warning_level > salt::WARNING_LEVEL)
		return;

	print_colored("warning: ", Color::YELLOW);
	std::cout << salt::file_names[current_file_name_index] << ':' << expr->line() << ':' << expr->col() << ": " << str << std::endl;
}

void salt::print_error(const std::string& str) {
	any_compile_error_occured = true;
	print_colored("error: ", Color::LIGHT_RED);
	std::cout << str << std::endl;
}

void salt::print_error_at(const Token& tok, const std::string& str) {
	any_compile_error_occured = true;
	print_colored("error: ", Color::LIGHT_RED);
	std::cout << salt::file_names[current_file_name_index] << ':' << tok.line() << ':' << tok.col() << ": " << str << std::endl;
}

void salt::print_error_at(ExprAST* expr, const std::string& str) {
	any_compile_error_occured = true;
	print_colored("error: ", Color::LIGHT_RED);
	std::cout << salt::file_names[current_file_name_index] << ':' << expr->line() << ':' << expr->col() << ": " << str << std::endl;
}


[[noreturn]]
void salt::print_fatal(const std::string& str, int exit_code) {
	any_compile_error_occured = true;
	print_colored("FATAL: ", Color::LIGHT_RED);
	std::puts(str.c_str());
	std::exit(exit_code);
}

[[noreturn]]
void salt::print_fatal(const char* str, int exit_code) {
	any_compile_error_occured = true;
	print_colored("FATAL: ", Color::LIGHT_RED);
	std::puts(str);
	std::exit(exit_code);
}


static uint64_t upow(uint64_t x, uint64_t y) {
	uint64_t res = 1;
	for (int i = 0; i < y; i++)
		res *= x;

	return res;
}

ParsedNumber salt::parse_num_literal(const std::string& _s, int radix) {
	constexpr char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	ParsedNumber ret_val = {};
	ret_val.type = PARSED_ERROR;
	ret_val.u64 = 0;

	if (_s.size() < 1 || _s.size() < 2 && _s[0] == '-') {
		ret_val.type = PARSED_BAD_NUMBER;
		return ret_val;
	}

	if (radix < 2 || radix > 36) {
		ret_val.type = PARSED_BAD_RADIX;
		return ret_val;
	}

	constexpr uint64_t max_uint = -1;

	bool negative = _s[0] == '-';
	std::string_view s = std::string_view(_s.c_str() + negative);

	// look at what we need to expect
	bool expect_float = false;
	if (s.find('.') != s.npos)
		expect_float = true;

	if (expect_float) {
		errno = 0;
		double res = std::strtod(_s.c_str(), nullptr);
		if (errno)
			ret_val.type = PARSED_ERROR;
		else
			ret_val.type = PARSED_FLOAT;
		ret_val.f64 = res;
		return ret_val;
	}


	// remember that s is the part of the number excluding the - sign
	size_t len = s.size();
	if (string_starts_with(s.data(), "0x") || string_starts_with(s.data(), "0X"))
		radix = 16;
	else if (string_starts_with(s.data(), "0b") || string_starts_with(s.data(), "0B"))
		radix = 2;
	else if (string_starts_with(s.data(), "0"))
		radix = 8;

	// expect an integer in base "radix"
	ret_val.type = negative ? PARSED_NEG_INT : PARSED_POS_INT;
	ret_val.u64 = 0;
	uint64_t res = 0;
	uint64_t old_res = 0;

	// optimization for radix <= 10
	if (radix <= 10) {
		uint64_t digit_value = 1;
		uint64_t old_digit_value = 1;

		int start_digit = 0;
		if (
			string_starts_with(s.data(), "0x")
			|| string_starts_with(s.data(), "0X")
			|| string_starts_with(s.data(), "0b")
			|| string_starts_with(s.data(), "0B")
			) {
			start_digit = 2;
		}

		for (int digit = s.size() - 1; digit >= start_digit; digit--) {

			// bad digit, for example digit 2 in binary literal
			if (s[digit] < '0' || s[digit] - '0' >= radix) {
				ret_val.type = PARSED_BAD_RADIX;
				return ret_val;
			}

			res += digit_value * uint64_t(s[digit] - '0');
			digit_value *= radix;



			// check overflow
			// if res overflowed, or 
			if (res < old_res || digit_value < old_digit_value) {
				ret_val.type = PARSED_OVERFLOW;
				return ret_val;
			}

			old_res = res;
			old_digit_value = old_digit_value;
		}

		if (negative) {
			if (res > uint64_t(INT64_MIN))
				ret_val.type = PARSED_OVERFLOW; // here underflow actually
			res = -res;
		}

		ret_val.u64 = res;
		
			
		return ret_val;
	}

	printf("Whoops, the radix of %s is %d!\n", s.data(), radix);
	TODO();
	return ret_val;
}