/*// no "pragma once"; this should only be included once and in testing.cpp!
// otherwise intentional compile error
// everything except add_t_Integer() should be private (in anonymous namespace).
#pragma once
#include "testing.h"
#include "../misc/integer.h"
#include <cstdlib>



namespace {
	const int iterations = 46350;
	const char* one_thousand_factorial = "402387260077093773543702433923003985719374864210714632543799910429938512398629020592044208486969404800479988610197196058631666872994808558901323829669944590997424504087073759918823627727188732519779505950995276120874975462497043601418278094646496291056393887437886487337119181045825783647849977012476632889835955735432513185323958463075557409114262417474349347553428646576611667797396668820291207379143853719588249808126867838374559731746136085379534524221586593201928090878297308431392844403281231558611036976801357304216168747609675871348312025478589320767169132448426236131412508780208000261683151027341827977704784635868170164365024153691398281264810213092761244896359928705114964975419909342221566832572080821333186116811553615836546984046708975602900950537616475847728421889679646244945160765353408198901385442487984959953319101723355556602139450399736280750137837615307127761926849034352625200015888535147331611702103968175921510907788019393178114194545257223865541461062892187960223838971476088506276862967146674697562911234082439208160153780889893964518263243671616762179168909779911903754031274622289988005195444414282012187361745992642956581746628302955570299024324153181617210465832036786906117260158783520751516284225540265170483304226143974286933061690897968482590125458327168226458066526769958652682272807075781391858178889652208164348344825993266043367660176999612831860788386150279465955131156552036093988180612138558600301435694527224206344631797460594682573103790084024432438465657245014402821885252470935190620929023136493273497565513958720559654228749774011413346962715422845862377387538230483865688976461927383814900140767310446640259899490222221765904339901886018566526485061799702356193897017860040811889729918311021171229845901641921068884387121855646124960798722908519296819372388642614839657382291123125024186649353143970137428531926649875337218940694281434118520158014123344828015051399694290153483077644569099073152433278288269864602789864321139083506217095002597389863554277196742822248757586765752344220207573630569498825087968928162753848863396909959826280956121450994871701244516461260379029309120889086942028510640182154399457156805941872748998094254742173582401063677404595741785160829230135358081840096996372524230560855903700624271243416909004153690105933983835777939410970027753472000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
	using namespace SaltTest;
	using namespace salt;
}


class t_Integer : public TestGroup {
public:
	static TestResult basic_add1() {
		Integer i = 30;
		Integer j = 60;
		Integer expected = 90;
		Integer res = i + j;
		if (res != expected)
			return TestResult(FAIL, "Failed addition between "
				+ std::to_string(30)
				+ " and "
				+ std::to_string(60)
				+ ": got result "
				+ reverse(res.str())
				+ ", expected "
				+ reverse(expected.str())
			);
		return TestResult(PASS);

		std::vector<int> vec;
	}

	static Integer factorial(const Integer& num) {
		if (num == Integer(1ll))
			return 1;
		if (num == Integer(2ll))
			return 2;

		return num * factorial(num - Integer(1ll));
	}

	static TestResult test_factorial() {
		const TestResult results[] = {
			test_for(factorial(1) == Integer(1ll), "failed to assert factorial of 1"),
			test_for(factorial(2) == Integer(2ll), "failed to assert factorial of 2"),
			test_for(factorial(3) == Integer(6ll), "failed to assert factorial of 3"),
			test_for(factorial(4) == Integer(24ll), "failed to assert factorial of 4"),
			test_for(factorial(5) == Integer(120ll), "failed to assert factorial of 5"),
			test_for(factorial(6) == Integer(720ll), "failed to assert factorial of 6"),
			test_for(factorial(7) == Integer(5040ll), "failed to assert factorial of 7"),
			test_for(factorial(29) == Integer(reverse("8841761993739701954543616000000")), "failed to assert factorial of 29"),
			test_for(factorial(55) == Integer(reverse("12696403353658275925965100847566516959580321051449436762275840000000000000")), "failed to assert factorial of 55"),
			test_for(factorial(1000) == Integer(reverse(one_thousand_factorial)), "failed to assert factorial of 1000"),
		};
		for (const TestResult& tr : results)
			if (tr.val() != PASS)
				return tr;

		return PASS;
	}


	static TestResult test_add() {
		for (int i = 0; i < iterations; i++) {
			// we generate a random number that can also be negative
			int rand_num = rand() % iterations - iterations / 2;
			int rand_num2 = rand() % iterations - iterations / 2;
			int sum = rand_num + rand_num2;

			Integer rand_Int = rand_num;
			Integer rand_Int2 = rand_num2;
			Integer Int_sum = rand_Int + rand_Int2;

			if (sum != Int_sum)
				return TestResult(FAIL, "Failed addition between "
					+ std::to_string(rand_num)
					+ " and "
					+ std::to_string(rand_num2)
					+ ": got result "
					+ reverse(Int_sum.str())
					+ ", expected "
					+ std::to_string(sum)
				);
		}

		return TestResult(PASS);
	}

	static TestResult test_sub() {
		for (int i = 0; i < iterations; i++) {
			// we generate a random number that can also be negative
			long long rand_num = rand() % iterations - iterations / 2;
			long long rand_num2 = rand() % iterations - iterations / 2;
			long long diff = rand_num - rand_num2;

			Integer rand_Int = Integer(rand_num);
			Integer rand_Int2 = Integer(rand_num2);
			Integer Int_diff = rand_Int - rand_Int2;

			if (diff != Int_diff)
				return TestResult(FAIL, "Failed subtraction between "
					+ std::to_string(rand_num)
					+ " and "
					+ std::to_string(rand_num2)
					+ ": got result "
					+ reverse(Int_diff.str())
					+ ", expected "
					+ std::to_string(diff)
				);
		}

		return TestResult(PASS);
	}

	static TestResult test_mul() {
		for (int i = 0; i < iterations; i++) {
			// we generate a random number that can also be negative
			long long rand_num = 500 + rand() % iterations - iterations / 2;
			long long rand_num2 = 500 + rand() % iterations - iterations / 2;
			long long prod = rand_num * rand_num2;

			Integer rand_Int = Integer(rand_num);
			Integer rand_Int2 = Integer(rand_num2);
			Integer Int_prod = rand_Int * rand_Int2;

			if (prod != Int_prod)
				return TestResult(FAIL, "Failed multiplication between "
					+ std::to_string(rand_num)
					+ " and "
					+ std::to_string(rand_num2)
					+ ": got result "
					+ reverse(Int_prod.str())
					+ ", expected "
					+ std::to_string(prod)
				);

		}

		return TestResult(PASS);
	}

	static TestResult test_integer_str() {
		Integer i = 30ll;
		Integer j = 40ll;
		Integer k = -20ll;

		std::string s_i = reverse(i.str());
		std::string s_j = reverse(j.str());
		std::string s_k = reverse(k.str());

		std::string exp_i = "30";
		std::string exp_j = "40";
		std::string exp_k = "-20";

		if (reverse(i.str()) != exp_i)
			return TestResult(FAIL, "Expected " + exp_i + ", got " + s_i);
		if (reverse(j.str()) != exp_j)
			return TestResult(FAIL, "Expected " + exp_j + ", got " + s_j);
		if (reverse(k.str()) != exp_k)
			return TestResult(FAIL, "Expected " + exp_k + ", got " + s_k);

		return PASS;
	}

	static TestResult test_constructor() {
		Integer i = Integer(20ll);
		return PASS;
	}

	static TestResult test_pass() {
		return PASS;
	}

	void register_tests() override {
		REGISTER_TEST(t_Integer::test_pass);
		/*
		REGISTER_TEST(t_Integer::test_integer_str);
		REGISTER_TEST(t_Integer::basic_add1);
		REGISTER_TEST(t_Integer::test_add);
		REGISTER_TEST(t_Integer::test_sub);
		REGISTER_TEST(t_Integer::test_mul);
		REGISTER_TEST(t_Integer::test_factorial);
		*/
		// REGISTER_TEST(t_Integer::test_constructor);
/*
	}

	const char* name() override { return "t_Integer"; }
};

void add_t_Integer() {
	all_test_groups.push_back(std::make_unique<t_Integer>());
}

*/