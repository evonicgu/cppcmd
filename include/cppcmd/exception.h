#ifndef CPPCMD_EXCEPTION_H
#define CPPCMD_EXCEPTION_H

#include <string>
#include <stdexcept>

namespace cppcmd::exception {

#define define_exception(type, inherit) \
    struct type : public inherit { \
        explicit type(const std::string& str) \
            : inherit(str) {} \
                                \
        explicit type(const char* str) \
            : inherit(str) {} \
    }

    define_exception(exception, std::runtime_error);
    
    namespace specification {

        define_exception(specification_exception, exception);

        define_exception(duplicate_command_name, specification_exception);
        define_exception(long_option_too_short, specification_exception);
        define_exception(long_option_invalid_name, specification_exception);
        define_exception(command_invalid_name, specification_exception);
        define_exception(short_option_invalid_name, specification_exception);
        define_exception(duplicate_long_option_name, specification_exception);
        define_exception(duplicate_short_option_name, specification_exception);

    }

    namespace parsing {

        define_exception(parsing_exception, exception);

        define_exception(long_option_too_short, parsing_exception);
        define_exception(long_option_invalid_name, parsing_exception);
        define_exception(short_option_invalid_name, parsing_exception);
        define_exception(option_value_missing, parsing_exception);
        define_exception(argument_value_missing, parsing_exception);
        define_exception(unrecognized_option_name, parsing_exception);
        define_exception(unrecognized_command_name, parsing_exception);
        define_exception(unable_to_parse_value, parsing_exception);
        define_exception(too_many_values, parsing_exception);
        define_exception(no_implicit_value, parsing_exception);
        define_exception(no_implicit_single_value, parsing_exception);
        define_exception(no_command_name, parsing_exception);
        define_exception(excessive_arguments, parsing_exception);
        define_exception(validation_error, parsing_exception);

    }

#undef define_exception

}

#endif //CPPCMD_EXCEPTION_H
