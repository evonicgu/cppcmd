#ifndef CPPCMD_MULTICOMMAND_APPLICATION_H
#define CPPCMD_MULTICOMMAND_APPLICATION_H

#include <memory>
#include <string>

#include "cppcmd/default_mapper.h"
#include "cppcmd/type_validator.h"
#include "cppcmd/parser/value/default_value_parser.h"
#include "cppcmd/command/command_dispatcher.h"

namespace cppcmd {

    namespace application {

        template<typename TOptions, typename TMapper = default_mapper<parser::default_value_parser>>
        class multicommand_application : private command::multi_command_dispatcher<TOptions, TMapper> {
            using TParser = typename TOptions::parser_t;

            static_assert(validate_options<TOptions>(), "Options type is invalid");

            TParser parser;
            TMapper mapper;

            typename multicommand_application::command_bag_t commands;

        public:
            explicit multicommand_application(TParser parser, TMapper mapper)
                : parser(std::move(parser)),
                  mapper(std::move(mapper)) {}

            template<typename T>
            void add_command(std::string name, T cmd) {
                if (!parser.validate_cmd_name(name)) {
                    throw exception::specification::command_invalid_name("Invalid command name: '" + name + "'");
                }

                if (commands.contains(name)) {
                    throw exception::specification::duplicate_command_name("Duplicate command name: '" + name + "'");
                }

                commands.emplace(std::move(name), std::make_unique<
                    typename multicommand_application::template any_command_erased<T>>(std::move(cmd)));
            }

            void parse(int argc, const char* const* argv) {
                this->parse_cmd(commands, parser, mapper, argc, argv);
            }
        };

    }

    template<typename TOptions, typename TMapper = default_mapper<parser::default_value_parser>>
    application::multicommand_application<TOptions, TMapper> multi_app(
        typename TOptions::parser_t parser = typename TOptions::parser_t{},
        TMapper mapper = TMapper{}) {
        return application::multicommand_application<TOptions, TMapper>(std::move(parser), std::move(mapper));
    }


}

#endif //CPPCMD_MULTICOMMAND_APPLICATION_H