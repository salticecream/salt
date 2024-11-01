#pragma once
static_assert(0, "Code shall not be resurrected in this way.");

static void test_all() {
    using namespace salt;
    std::string test_numbers[] = {
        "18446744073709551615",
        "18446744073709551616",
        "1844674407370955161512783621873123127645",
        "-9223372036854775809",
        "-123171632813623467814612412",
        "-9223372036854775808",
        "-1",
        "0",
        "-0",
        "NaN",
        "nan",
        "1.1273e+4",
        "1,1273e+4",
        "1.1273e4",
        "1,1273e4",
        "1.8e309",
        "inf"
        "fuck"
    };

    for (std::string& s : test_numbers) {
        ParsedNumber pn = parse_num_literal(s);
        switch (pn.type) {
        case PARSED_FLOAT:
            printf("Parsed `%s` as %lf", s.c_str(), pn.f64);
            break;
        case PARSED_NEG_INT:
            printf("Parsed `%s` as %lld", s.c_str(), (int64_t)pn.u64);
            break;
        case PARSED_POS_INT:
            printf("Parsed `%s` as %llu", s.c_str(), pn.u64);
            break;
        case PARSED_BAD_NUMBER:
            printf("`%s` - error: Bad number", s.c_str());
            break;
        case PARSED_BAD_RADIX:
            printf("`%s` - error: Bad radix", s.c_str());
            break;
        case PARSED_ERROR:
            printf("`%s` - error!!!", s.c_str());
            break;
        case PARSED_OVERFLOW:
            printf("`%s` - error - overflow", s.c_str());
            break;
        }
        std::cout << std::endl;
    }

}