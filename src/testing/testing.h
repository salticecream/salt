#pragma once
#include "../common.h"



namespace SaltTest
{


	enum TestValue {
		NONE = -2,
		EXCEPTION = -1,
		FAIL,
		PASS,
	};

	class TestResult {
	private:
		TestValue tv_;
		std::string what_;
	public:
		std::string what() const { return what_; }
		TestValue val() const { return tv_; }
		TestResult(TestValue tv) : tv_(tv), what_(tv == FAIL ? "Unspecified failure" : "") {}
		TestResult(TestValue tv, const std::string& f_what) : what_(f_what), tv_(tv) {}
	};



	class TestGroup {
	protected:
		std::vector<std::tuple<TestResult(*)(void), const char*, TestValue>> tests_;
	public:

		virtual const char* name() = 0;

		// override with a function containing only REGISTER_TEST(x) and REGISTER_FAIL(x) and REGISTER_EXCEPTION(x)
		virtual void register_tests() = 0;

		virtual ~TestGroup() = default;
		const std::vector<std::tuple<TestResult(*)(void), const char*, TestValue>>& tests() { return tests_; }
	};

	TestResult test_for(bool i, const std::string& error_message) {
		if (i)
			return PASS;

		return TestResult(FAIL, error_message);
	}

	struct TestPair {
		int expected;
		int unexpected;
	};

	std::vector<int> large_vector(size_t size) {
		std::vector<int> res = std::vector<int>(size, 0);
		return res;
	}

	std::vector<int> large_random_vector(size_t size) {
		auto res = large_vector(size);
		for (int i = 0; i < size; i++)
			res[i] = rand();
		return res;
	}

	extern std::vector<std::unique_ptr<TestGroup>> all_test_groups;

	unsigned long long time_func(void (*f)());
}



#define REGISTER_TEST(t) tests_.push_back(std::make_tuple(t, #t, TestValue::PASS))
#define REGISTER_FAIL(t) tests_.push_back(std::make_tuple(t, #t, TestValue::FAIL))
#define REGISTER_EXCEPTION(t) tests_.push_back(std::make_tuple(t, #t, TestValue::EXCEPTION))
#define ADD_TEST_GROUP(c) all_test_groups.push_back(std::make_unique<c>())


