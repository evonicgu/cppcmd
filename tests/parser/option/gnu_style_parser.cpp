#include "gtest/gtest.h"

#include "cppcmd/options.h"
#include "cppcmd/parser/option/gnu_style_parser.h"
#include "cppcmd/typedefs.h"

#include "test_args.h"

namespace cppcmd::tests::option_parser_tests {

    class gnu_style_parser_test_fixture :
        public ::testing::Test {
    protected:
        parser::gnu_style_parser parser;
    };

    struct invalid_long_option_name : cppcmd::options {
        option<int> opt{
            config::long_name{"123"}
        };
    };

    CPPCMD_OPTIONS(invalid_long_option_name, opt)

    TEST_F(gnu_style_parser_test_fixture, invalid_long_option_name) {
        EXPECT_THROW(parser.create_prototype(invalid_long_option_name{}), exception::specification::long_option_invalid_name);
    }

    struct duplicate_long_option_name : cppcmd::options {
        option<int> opt1{
            config::long_name{"asdf23"}
        };
        option<int> opt2{
            config::long_name{"asdf23"}
        };
    };

    CPPCMD_OPTIONS(duplicate_long_option_name, opt1, opt2)

    TEST_F(gnu_style_parser_test_fixture, duplicate_long_option_name) {
        EXPECT_THROW(parser.create_prototype(duplicate_long_option_name{}), exception::specification::duplicate_long_option_name);
    }

    struct invalid_short_option_name : cppcmd::options {
        option<int> opt1{
            config::short_name{'1'}
        };
    };

    CPPCMD_OPTIONS(invalid_short_option_name, opt1)

    TEST_F(gnu_style_parser_test_fixture, invalid_short_option_name) {
        EXPECT_THROW(parser.create_prototype(invalid_short_option_name{}), exception::specification::short_option_invalid_name);
    }

    struct duplicate_short_option_name : cppcmd::options {
        option<int> opt1{
            config::short_name{'c'}
        };

        option<int> opt2{
            config::short_name{'c'}
        };
    };

    CPPCMD_OPTIONS(duplicate_short_option_name, opt1, opt2)

    TEST_F(gnu_style_parser_test_fixture, duplicate_short_option_name) {
        EXPECT_THROW(parser.create_prototype(duplicate_short_option_name{}), exception::specification::duplicate_short_option_name);
    }

    class gnu_style_parser_test_cmd_names_fixture :
        public ::testing::TestWithParam<std::pair<std::string, bool>> {
    protected:
        parser::gnu_style_parser parser;
    };

    TEST_P(gnu_style_parser_test_cmd_names_fixture, validate_cmd_name_test) {
        auto& [name, expected] = GetParam();

        EXPECT_EQ(expected, parser.validate_cmd_name(name));
    }

    std::pair<std::string, bool> validate_cmd_name_values[] = {
        {"valid_cmd", true},
        {"valid-cmd", true},
        {"1valid-cmd", false},
        {"-invalid-cmd", false},
        {"1111", false},
        {"$valid", true},
        {"___", true},
        {"----", false},
        {"", false}
    };

    INSTANTIATE_TEST_SUITE_P(
        gnu_style_parser_validate_cmd_name,
        gnu_style_parser_test_cmd_names_fixture,
        ::testing::ValuesIn(validate_cmd_name_values));



}
