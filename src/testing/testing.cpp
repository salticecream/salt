#include "../common.h"
#include "testing.h"
#include <chrono>
#include <Windows.h>
#include <csignal>
#include "t_common.h"
#include "t_testing.h"

using namespace SaltTest;
using namespace salt;
using namespace Windows;

namespace chrono = std::chrono;

#define FUKSALTEX
#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define WHITE (FOREGROUND_WHITE | FOREGROUND_INTENSITY)
#define GREEN (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#define RED (FOREGROUND_RED | FOREGROUND_INTENSITY)

std::vector<std::unique_ptr<TestGroup>> SaltTest::all_test_groups;

static void add_all_tests() {
	add_t_testing();
	add_t_common();
}

static void set_color(int color) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);   
}

static long long ms_diff(const chrono::steady_clock::time_point t1, const chrono::steady_clock::time_point t2) {
	return chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
}

unsigned long long time_func(void (*f)()) {
	auto start = chrono::steady_clock::now();
	f();
	auto end = chrono::steady_clock::now();
	return chrono::duration_cast<chrono::milliseconds>(end - start).count();
}

static const char* current_function = nullptr;
static void segmentation_fault_handler(int /*signal*/) {
	printf("\nFATAL: Segmentation fault occured at function: %s\n", current_function ? current_function : "(NULL)");
}


 
int main() {
	TestPair passes{ 0, 0 };
	TestPair fails{ 0, 0 };
	TestPair exceptions{ 0, 0 };
	std::signal(SIGSEGV, segmentation_fault_handler);

	auto start_time = chrono::steady_clock::now();

	add_all_tests();
	for (auto& tg : all_test_groups)
		tg->register_tests();
	auto test_loaded_time = chrono::steady_clock::now();
	std::cout << "Initialized all tests in " << ms_diff(start_time, test_loaded_time) << " ms" << std::endl;



	for (auto& test_group : all_test_groups) {
		std::cout << std::endl;
		set_color(FOREGROUND_WHITE);

		auto test_group_start_time = chrono::steady_clock::now();
		for (auto& test_fn_tuple : test_group->tests()) {
			TestResult test_result = TestResult(NONE);

			auto test_fn = std::get<0>(test_fn_tuple);
			const char* test_name = std::get<1>(test_fn_tuple);
			current_function = test_name;
			TestValue expected_value = std::get<2>(test_fn_tuple);
			std::string exception;

			try {
				test_result = test_fn();
			}
			catch (const salt::Exception& e) {
				exception = e.what();
				test_result = EXCEPTION;
			}


			switch (test_result.val()) {
			case PASS:
				set_color(expected_value == PASS ? GREEN : RED);
				std::cout << "PASS: ";
				set_color(FOREGROUND_WHITE);
				std::cout << test_name << "()";
				if (expected_value == PASS)
					passes.expected++;
				else {
					passes.unexpected++;
					std::cout << " [unexpected]";
				}
				std::cout << std::endl;
				break;

			case FAIL:
				set_color(expected_value == FAIL ? GREEN : RED);
				std::cout << "FAIL: ";
				set_color(FOREGROUND_WHITE);
				std::cout << test_name << "(): " << test_result.what();
				if (expected_value == FAIL) {
					fails.expected++;
					std::cout << " [expected]";
				}
				else {
					fails.unexpected++;
				}
				std::cout << std::endl;
				break;

			case EXCEPTION:
				set_color(expected_value == EXCEPTION ? GREEN : RED);
				std::cout << "EXCEPTION: ";
				set_color(FOREGROUND_WHITE);
				std::cout << test_name << "(): " << exception;
				if (expected_value == EXCEPTION) {
					exceptions.expected++;
					std::cout << " [expected]";
				}
				else {
					exceptions.unexpected++;
				}
				std::cout << std::endl;
				break;
			default:
				set_color(FOREGROUND_RED);
				std::cout << "fatal error in test suite: test_result.val() was not PASS, FAIL or EXCEPTION";
				set_color(0xf); // white
				return 2;
			}
		}
		set_color(FOREGROUND_RED | FOREGROUND_GREEN);
		std::cout << std::endl << test_group->name();
		set_color(FOREGROUND_WHITE);
		std::cout << ": performed " << test_group->tests().size() << " tests in " << ms_diff(test_group_start_time, chrono::steady_clock::now()) << " ms" << std::endl;
	}

	std::cout << std::endl << std::endl;
	std::printf("passes: \t%d expected | %d unexpected\n", passes.expected, passes.unexpected);
	std::printf("fails:  \t%d expected | %d unexpected\n", fails.expected, fails.unexpected);
	std::printf("exceptions: \t%d expected | %d unexpected\n\n", exceptions.expected, exceptions.unexpected);
	std::printf("finished testing in %lld ms", ms_diff(start_time, chrono::steady_clock::now()));
	std::cout << std::endl;

	if (!passes.unexpected && !fails.unexpected && !exceptions.unexpected) {
		std::cout << "all tests working as intended " << std::endl;
		return 0;
	} else {
		std::cout << "some tests failed..." << std::endl;
		return 1;
	}
			

	

}
