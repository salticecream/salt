#pragma once
#include "testing.h"

namespace {
	using namespace salt;
	using namespace SaltTest;
	static const int T_TESTING_ITERATIONS = 1000;
}

static class t_testing : public TestGroup {

	static TestResult test_large_vector() {
		auto lv = large_vector(300000);
		for (const int& i : lv)
			if (i != 0)
				return TestResult(FAIL, "large_vector return a vector of zeroes!");
		return PASS;
	}

	static TestResult test_large_random_vector() {
		auto lv = large_random_vector(300000);
		for (const int& i : lv)
			if (i != 0)
				return PASS;
		TestResult(FAIL, "large_vector return a vector of zeroes!");
	}

	static TestResult test_pass() {
		return PASS;
	}

	static TestResult test_fail() {
		return TestResult(FAIL, "test");
	}

	static TestResult test_exception() {
		throw Exception("test");
	}

	void register_tests() override {
		REGISTER_TEST(t_testing::test_large_vector);
		REGISTER_TEST(t_testing::test_large_random_vector);
		REGISTER_TEST(t_testing::test_pass);
		REGISTER_FAIL(t_testing::test_fail);
		REGISTER_EXCEPTION(t_testing::test_exception);
	}

	const char* name() override { 
		return "t_testing"; 
	}
};

void add_t_testing() {
	ADD_TEST_GROUP(t_testing);
}