#pragma once
#include "testing.h"
#include <cstdlib> // for rand() function; it's pseudorandom so always gives same results
#include <chrono>

namespace {
	using namespace SaltTest;
	using namespace salt;
	namespace chrono = std::chrono;
	const int T_COMMON_ITERATIONS = 1000;
}

static class t_common : public TestGroup {
	static TestResult test_atoi() {
		const char* const error_fmt = "atoi with string \"%.11s\" produced different results: %d and %d";

		char number_buffer[12] = "";
		for (int i = 0; i < T_COMMON_ITERATIONS; i++) {
			itoa(rand(), number_buffer, 10);
			if (std::atoi(number_buffer) != salt::atoi(number_buffer))
				return TestResult(FAIL, f_string(error_fmt, number_buffer, std::atoi(number_buffer), salt::atoi(number_buffer)));
		}

		// INT_MIN, 0, INT_MAX
		for (const char* num_str : { "-2147483648", "0", "2147483647" }) {
			if (std::atoi(num_str) != salt::atoi(num_str))
				return TestResult(FAIL, f_string(error_fmt, number_buffer, std::atoi(number_buffer), salt::atoi(number_buffer)));
		}

		return PASS;
	}

	// Assert that to_string() properly formats and outputs the contents of a vector.
	static TestResult test_vec_to_str() {
		std::vector<int> vec = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
		std::vector<const char*> vec2 = { "abc", "def", "ghi" };
		
		std::string vec1_str1 = "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10]";
		std::string vec1_str2 = salt::to_string(vec);

		std::string vec2_str1 = "[abc, def, ghi]";
		std::string vec2_str2 = salt::to_string(vec2);

		if (vec1_str1 != vec1_str2)
			return TestResult(FAIL, f_string("Failed to_string(): Expected %s, got %s", vec1_str1.c_str(), vec1_str2.c_str()));

		if (vec2_str1 != vec2_str2)
			return TestResult(FAIL, f_string("Failed to_string(): Expected %s, got %s", vec2_str1.c_str(), vec2_str2.c_str()));

		return PASS;
	}

	// Assert that unwrap() returns the correct value.
	static TestResult test_Result_unwrap_value() {
		salt::Result<int> res = salt::Result<int>(salt::Result_e::OK, 277135);
		
		int i1 = 277135, i2 = res.unwrap();
		
		salt::Result<std::string> res2 = std::string("Hello!");

		std::string s1 = "Hello!", s2 = res2.unwrap();

		if (i1 != i2)
			return TestResult(FAIL, salt::f_string("unwrap() got different values: i1 = %d, i2 = %d", i1, i2));

		if (s1 != s2)
			return TestResult(FAIL, salt::f_string("unwrap() got different values: i1 = `%s`, i2 = `%s`", s1.c_str(), s2.c_str()));

		// std::cout << f_string("&i1 = %p\n&i2 = %p\n&i3 = %p\n", &i1, &i2, &i3);
		return PASS;
	}

	// Assert that "val" in expressions of the form "T val = res.unwrap()" takes ownership of the data within "res".
	static TestResult test_Result_unwrap_move() {
		std::vector<int> vec1 = {1, 2, 3, 4, 5};

		const void* volatile address1 = &vec1[0];
		
		
		// This is always true (obviously) but the compiler will hopefully be
		// tricked into dropping res out of scope
		if (static_cast<volatile int> (1)) {
			salt::Result<std::vector<int>> res = salt::Result(salt::Result_e::OK, std::vector<int>({ 6, 7, 8, 9, 10 }));
			vec1 = res.unwrap();
		}

		// We now assert that vec1 took ownership of the memory within res
		// This is done by checking that the vector's internal ptr now points to res' data
		const void* volatile address2 = &vec1[0];

		if (address1 == address2)
			return TestResult(FAIL, "return value of unwrap() was not treated as an rvalue reference");

		return test_for(vec1 == std::vector<int>({ 6, 7, 8, 9, 10 }), f_string("unwrap() got different values for the vectors. \nexpected: %s\ngot: %s",
			salt::to_string(vec1).c_str(), salt::to_string(std::vector<int>{6, 7, 8, 9, 10}).c_str()));
	}

	// Assert that OK results evaluate to true, and ERR results evaluate to false.
	static TestResult test_Result_bool() {
		Result<void> ok = Result<void>(Result_e::OK);
		Result<int> err = Result<int>(Result_e::ERR, Exception("testception"));
		return test_for(
			bool(ok) == true &&
			bool(ok) != false &&
			bool(err) == false &&
			bool(err) != true,
			"failed bool conversion of Result"
		);
	}

	// Assert that res.unwrap_or(x) is res.unwrap() if res is OK, and x if res is ERR.
	static TestResult test_Result_unwrap_or() {
		const int default_int = 0;
		for (int i = 0; i < T_COMMON_ITERATIONS; i++) {
			unsigned non_zero = 1 + static_cast<unsigned>(rand() >> 1);
			Result_e result_type = non_zero < UINT_MAX / 4 ? Result_e::OK : Result_e::ERR;
			Result<unsigned> res =
				result_type == Result_e::OK ?
				Result<unsigned>(Result_e::OK, std::move(non_zero)) :
				Result<unsigned>(Result_e::ERR, "Bad result");
			
			unsigned final_number = res.unwrap_or(0);

			if (res && final_number == 0)
				return TestResult(FAIL, "Unexpected value 0 for nonzero number; did unwrap_or() fail?");
			if (!res && final_number != 0)
				return TestResult(FAIL, f_string("Unexpected value %d instead of 0; did unwrap_or() fail?", final_number));

		}
		return PASS;
	}

	// Assert that unwrap() does not move the data in memory, but rather moves ownership from one owner to another.
	static TestResult test_Result_unwrap() {
		// Create a large vector
		std::vector<int> vec = large_random_vector(10000000);

		// Check where the data is stored
		void* addr1 = &vec[0];

		// Wrap this vector inside a Result (this vector must not be copied, just wrapped)
		Result<std::vector<int>> res = Result(Result_e::OK, std::move(vec));

		// Unwrap this vector, it shouldn't have moved in memory
		std::vector<int> vec2 = res.unwrap();

		// Check where the data is stored now
		void* addr2 = &vec2[0];

		// Assert that the data didn't move
		return test_for(addr1 == addr2, f_string("unwrap() copied some data! Address diff: %llu", reinterpret_cast<size_t>(addr1) - reinterpret_cast<size_t>(addr2)));
		
	}

	// Assert that constructing a Result from an rvalue (even if that rvalue is a cast by std::move)
	// does not copy any data (and is therefore fast).
	static TestResult test_Result_wrap() {
		// Create a 4gb vector
		std::vector<int> lv = large_vector(1000000000);

		// Assert that it takes less than 10ms to wrap this vector
		auto start = chrono::steady_clock::now();
		Result<std::vector<int>> rv = std::move(lv);
		auto end = chrono::steady_clock::now();

		unsigned long long time_elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count();
		return test_for(time_elapsed < 10, f_string("Wrapping large vector took %llu ms, expected <10 ms", time_elapsed));

	}

	void register_tests() override {
		REGISTER_TEST(t_common::test_atoi);
		REGISTER_TEST(t_common::test_vec_to_str);
		REGISTER_TEST(t_common::test_Result_unwrap_move);
		REGISTER_TEST(t_common::test_Result_bool);
		REGISTER_TEST(t_common::test_Result_unwrap_or);
		REGISTER_TEST(t_common::test_Result_unwrap);
		REGISTER_TEST(t_common::test_Result_wrap);
	}

	const char* name() override {
		return "t_common";
	}
};

void add_t_common() {
	ADD_TEST_GROUP(t_common);
}