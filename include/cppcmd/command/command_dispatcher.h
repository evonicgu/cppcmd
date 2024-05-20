#ifndef COMMAND_DISPATCHER_H
#define COMMAND_DISPATCHER_H

#include <string>
#include <memory>

#include "cppcmd/default_mapper.h"
#include "cppcmd/type_validator.h"

namespace cppcmd::command {

    namespace detail {

        struct no_argument_helper {
        };

    }

    template<typename TOptions, typename TArguments>
    struct simple_parse_result {
        TOptions options;
        TArguments arguments;
        std::string program_name;
        unmatched_data unmatched;
    };

    template<typename TOptions>
    struct command_frame {
        TOptions options;
        std::string program_name;
        unmatched_data unmatched;

        command_frame(TOptions&& options, std::string program_name, unmatched_data&& unmatched)
            : options(std::move(options)),
              program_name(std::move(program_name)),
              unmatched(std::move(unmatched)) {
        }
    };

    template<typename TOptions>
    class command_dispatcher {
        static_assert(validate_options<TOptions>(), "Options type is invalid");

    protected:
        using TParser = typename TOptions::parser_t;

        std::optional<typename TParser::options_prototype_t> cached_prototype;

        void init_prototype(const TOptions& options, const TParser& parser) {
            if (!this->cached_prototype.has_value()) {
                auto prototype = parser.create_prototype(options);

                this->cached_prototype = std::move(prototype);
            }
        }
    };

    template<typename TOptions, typename TArguments>
    class single_command_dispatcher : public command_dispatcher<TOptions> {
        static_assert(std::is_same_v<typename TOptions::parser_t, typename TArguments::parser_t>,
                "Options and Arguments types must have the same parser type");
        static_assert(validate_arguments<TArguments>(), "Arguments type is invalid");

    protected:
        template<typename TMapper>
        simple_parse_result<TOptions, TArguments> parse_cmd(const typename single_command_dispatcher::TParser& parser,
            const TMapper& mapper,
            int argc, const char* const* argv) {
            TOptions options;
            TArguments args;

            this->init_prototype(options, parser);

            auto program_name = argv[0];

            auto parse_result = parser.parse(argc - 1, argv + 1, this->cached_prototype.value(),
                parser::argument_limit::unlimited);

            auto unmatched = mapper.template map<TOptions, TArguments>(options, args, parse_result);

            return simple_parse_result<TOptions, TArguments>{
                std::move(options),
                std::move(args),
                program_name,
                unmatched
            };
        }
    };

    template<typename TOptions>
    class multi_command_dispatcher : public command_dispatcher<TOptions> {
    protected:
        typename multi_command_dispatcher::TParser p;

        template<typename TMapper, typename ... TPrevious>
        class any_command {
            static_assert((std::is_rvalue_reference_v<TPrevious&&> && ...), "All previous types must be rvalue-references");

        public:
            virtual void invoke(const typename multi_command_dispatcher::TParser&,
                const TMapper&, int, const char* const*, TPrevious&& ...) = 0;

            virtual ~any_command() = default;
        };

        template<typename TCommand, typename TMapper, typename ... TPrevious>
        class any_command_erased : public any_command<TMapper, TPrevious ...> {
            TCommand cmd;

        public:
            explicit any_command_erased(TCommand&& cmd)
                : cmd(std::move(cmd)) {}

            void invoke(const typename multi_command_dispatcher::TParser& parser,
                const TMapper& mapper, int argc, const char* const* argv,
                TPrevious&& ... previous) override {
                cmd.invoke(parser, mapper, argc, argv, std::move(previous) ...);
            }
        };

        template<typename TMapper, typename ... TPrevious>
        using command_bag_t = std::unordered_map<std::string, std::unique_ptr<any_command<TMapper, TPrevious ...>>>;

        template<typename TMapper, typename ... TPrevious>
        void parse_cmd(const command_bag_t<TMapper, TPrevious ..., command_frame<TOptions>>& commands,
            const typename multi_command_dispatcher::TParser& parser,
            const TMapper& mapper, int argc, const char* const* argv, TPrevious&& ... previous) {
            static_assert((std::is_rvalue_reference_v<TPrevious&&> && ...), "All previous types must be rvalue-references");

            TOptions options;
            detail::no_argument_helper args;

            this->init_prototype(options, parser);

            auto program_name = argv[0];

            parser::parsing_result parse_result = parser.parse(argc - 1, argv + 1, this->cached_prototype.value(),
                parser::argument_limit::single);

            if (parse_result.arguments.empty()) {
                throw exception::parsing::no_command_name("No command line provided!");
            }

            std::string cmd_name = std::move(parse_result.arguments[0]);
            parse_result.arguments.clear();

            auto cmd_it = commands.find(cmd_name);

            if (cmd_it == commands.end()) {
                throw exception::parsing::unrecognized_command_name("Unknown command name: '" + cmd_name + '\'');
            }

            auto unmatched = mapper.template map<TOptions, detail::no_argument_helper>(options, args, parse_result);

            cmd_it->second->invoke(parser, mapper, argc - parse_result.args_used, argv + parse_result.args_used,
                std::move(previous) ...,
                command_frame<TOptions>{
                    std::move(options),
                    program_name,
                    std::move(unmatched)
                });
        }
    };

}

#endif //COMMAND_DISPATCHER_H
