#include "gtest/gtest.h"

#include "cppcmd/parser/value/default_value_parser.h"

namespace cppcmd::tests::value_parser_tests {

    class default_value_parser_test_fixture :
        public ::testing::Test {
    protected:
        parser::default_value_parser value_parser;
    };

    TEST_F(default_value_parser_test_fixture, positive_parse_test) {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;

        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;

        std::string_view one = "1";

        EXPECT_EQ(1, (value_parser.parse(one, i8), i8));
        EXPECT_EQ(1, (value_parser.parse(one, i16), i16));
        EXPECT_EQ(1, (value_parser.parse(one, i32), i32));
        EXPECT_EQ(1, (value_parser.parse(one, i64), i64));

        EXPECT_EQ(1, (value_parser.parse(one, u8), u8));
        EXPECT_EQ(1, (value_parser.parse(one, u16), u16));
        EXPECT_EQ(1, (value_parser.parse(one, u32), u32));
        EXPECT_EQ(1, (value_parser.parse(one, u64), u64));

        std::string_view requires_64_bits = "5000000000";

        EXPECT_THROW(value_parser.parse(requires_64_bits, i8), exception::parsing::unable_to_parse_value);
        EXPECT_THROW(value_parser.parse(requires_64_bits, i16), exception::parsing::unable_to_parse_value);
        EXPECT_THROW(value_parser.parse(requires_64_bits, i32), exception::parsing::unable_to_parse_value);
        EXPECT_EQ(5'000'000'000, (value_parser.parse(requires_64_bits, i64), i64));

        EXPECT_THROW(value_parser.parse(requires_64_bits, u8), exception::parsing::unable_to_parse_value);
        EXPECT_THROW(value_parser.parse(requires_64_bits, u16), exception::parsing::unable_to_parse_value);
        EXPECT_THROW(value_parser.parse(requires_64_bits, u32), exception::parsing::unable_to_parse_value);
        EXPECT_EQ(5'000'000'000, (value_parser.parse(requires_64_bits, u64), u64));
    }

    TEST_F(default_value_parser_test_fixture, integral_garbage_parse_test) {
        int8_t i8;

        std::string_view text_garbage = "words";

        EXPECT_THROW(value_parser.parse(text_garbage, i8), exception::parsing::unable_to_parse_value);

        std::string_view text_extraneous = "1aaa";

        EXPECT_THROW(value_parser.parse(text_extraneous, i8), exception::parsing::unable_to_parse_value);
    }

    TEST_F(default_value_parser_test_fixture, negative_parse_test) {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;

        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;

        std::string_view one = "-1";

        EXPECT_EQ(-1, (value_parser.parse(one, i8), i8));
        EXPECT_EQ(-1, (value_parser.parse(one, i16), i16));
        EXPECT_EQ(-1, (value_parser.parse(one, i32), i32));
        EXPECT_EQ(-1, (value_parser.parse(one, i64), i64));

        EXPECT_THROW(value_parser.parse(one, u8), exception::parsing::unable_to_parse_value);
        EXPECT_THROW(value_parser.parse(one, u16), exception::parsing::unable_to_parse_value);
        EXPECT_THROW(value_parser.parse(one, u32), exception::parsing::unable_to_parse_value);
        EXPECT_THROW(value_parser.parse(one, u64), exception::parsing::unable_to_parse_value);
    }

    TEST_F(default_value_parser_test_fixture, string_parse_test) {
        std::string str;
        std::string_view text = "text";

        EXPECT_EQ(text, (value_parser.parse(text, str), str));
    }

    TEST_F(default_value_parser_test_fixture, char_parse_test) {
        std::string_view text_c = "c";
        std::string_view text_cc = "cc";

        char c;

        EXPECT_EQ('c', (value_parser.parse(text_c, c), c));
        EXPECT_THROW(value_parser.parse(text_cc, c), exception::parsing::unable_to_parse_value);
    }

    TEST_F(default_value_parser_test_fixture, optional_parse_test) {
        std::optional<int8_t> opt_i8;
        std::string_view present = "12";
        std::string_view empty;

        EXPECT_EQ(12, (value_parser.parse(present, opt_i8), opt_i8.value()));
        EXPECT_FALSE((value_parser.parse(empty, opt_i8), opt_i8.has_value()));
    }

    class default_value_parser_bool_test_fixture :
        public ::testing::TestWithParam<std::pair<std::string_view, bool>> {
    protected:
        parser::default_value_parser value_parser;
    };

    std::pair<std::string_view, bool> parse_bool_values[] = {
        {"T", true},
        {"t", true},
        {"1", true},
        {"F", false},
        {"f", false},
        {"0", false},
        {"True", true},
        {"true", true},
        {"False", false},
        {"false", false},
    };

    TEST_P(default_value_parser_bool_test_fixture, bool_parse_test) {
        auto [str, expected] = GetParam();

        bool result;

        value_parser.parse(str, result);

        EXPECT_EQ(expected, result);
    }

    INSTANTIATE_TEST_SUITE_P(
        default_value_parser_bool_parse,
        default_value_parser_bool_test_fixture,
        ::testing::ValuesIn(parse_bool_values));

    class default_value_parser_invalid_bool_test_fixture :
        public ::testing::TestWithParam<std::string_view> {
    protected:
        parser::default_value_parser value_parser;
    };

    std::string_view invalid_parse_bool_values[] = {
        "garbage",
        "c",
        "a",
        "tRuE",
        "True ",
        "FalSe",
        "falsee"
    };

    TEST_P(default_value_parser_invalid_bool_test_fixture, invalid_bool_parse_test) {
        auto str = GetParam();

        bool result;

        EXPECT_THROW(value_parser.parse(str, result), exception::parsing::unable_to_parse_value);
    }

    INSTANTIATE_TEST_SUITE_P(
        default_value_parser_invalid_bool_parse,
        default_value_parser_invalid_bool_test_fixture,
        ::testing::ValuesIn(invalid_parse_bool_values));

    class default_value_parser_container_test_fixture :
        public ::testing::TestWithParam<std::tuple<char, std::string_view, std::vector<std::string>>> {
    protected:
        parser::default_value_parser value_parser;

        void SetUp() override {
            value_parser = parser::default_value_parser{parser::default_value_parser_config{
                .value_separator = std::get<0>(GetParam())
            }};
        }
    };

    TEST_P(default_value_parser_container_test_fixture, container_parse_test) {
        auto& [_, str, values] = GetParam();

        std::vector<std::string> parsed_vector;

        value_parser.parse(str, parsed_vector);

        EXPECT_EQ(values, parsed_vector);

        std::set<std::string> values_set{values.begin(), values.end()};

        std::set<std::string> parsed_set;

        value_parser.parse(str, parsed_set);

        EXPECT_EQ(values_set, parsed_set);
    }

    std::tuple<char, std::string_view, std::vector<std::string>> container_parse_values[] = {
        {',', "value1,value2", {"value1", "value2"}},
        {'.', "value1.value2", {"value1", "value2"}},
        {',', "value1.value2", {"value1.value2"}},
        {',', "value1,value2,value3,value4", {"value1", "value2", "value3", "value4"}},
        {'-', "value1-value2-value3", {"value1", "value2", "value3"}}
    };

    INSTANTIATE_TEST_SUITE_P(
        default_value_parser_container_parse,
        default_value_parser_container_test_fixture,
        ::testing::ValuesIn(container_parse_values));

}