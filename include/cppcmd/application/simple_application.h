#ifndef CPPCMD_SIMPLE_APPLICATION_H
#define CPPCMD_SIMPLE_APPLICATION_H

#include <type_traits>

#include "cppcmd/parser/value/default_value_parser.h"
#include "cppcmd/default_mapper.h"
#include "cppcmd/type_validator.h"
#include "cppcmd/command/command_dispatcher.h"

namespace cppcmd {

    namespace application {

        template<typename TOptions, typename TArguments,
            typename TMapper = default_mapper<parser::default_value_parser>>
        class simple_application : public command::single_command_dispatcher<TOptions, TArguments> {
            using TParser = typename TOptions::parser_t;

            TParser parser;
            TMapper mapper;

            static_assert(std::is_same_v<typename TOptions::parser_t, typename TArguments::parser_t>,
                    "Options and Arguments types must have the same parser type");
            static_assert(validate_options<TOptions>(), "Options type is invalid");
            static_assert(validate_arguments<TArguments>(), "Arguments type is invalid");

        public:
            explicit simple_application(TParser parser, TMapper mapper)
                : parser(std::move(parser)),
                  mapper(std::move(mapper)) {}

            command::simple_parse_result<TOptions, TArguments> parse(int argc, const char* const* argv) {
                return this->parse_cmd(parser, mapper, argc, argv);
            }
        };

    }

    template<typename TOptions, typename TArguments, typename TMapper = default_mapper<parser::default_value_parser>>
    application::simple_application<TOptions, TArguments, TMapper> simple_app(
        typename TOptions::parser_t parser = typename TOptions::parser_t{},
        TMapper mapper = TMapper{}) {
        return application::simple_application<TOptions, TArguments, TMapper>(std::move(parser), std::move(mapper));
    }

}

#endif //CPPCMD_SIMPLE_APPLICATION_H
