#ifndef CPPCMD_COMMAND_H
#define CPPCMD_COMMAND_H

#include <type_traits>

#include "command_dispatcher.h"
#include "cppcmd/type_validator.h"

namespace cppcmd::command {

    template<typename TOptions, typename ... TCommands>
    class forward_command {};

    template<typename TOptions, typename ... TCommands>
    forward_command<TOptions, TCommands ...> forward(std::pair<std::string, TCommands>&& ... cmds) {

    }

    template<typename TInherit, typename TOptions, typename TArguments>
    class regular_command : public single_command_dispatcher<TOptions, TArguments> {
        using TParser = typename TOptions::parser_t;

        static_assert(std::is_same_v<typename TOptions::parser_t, typename TArguments::parser_t>,
                "Options and Arguments types must have the same parser type");
        static_assert(validate_options<TOptions>(), "Options type is invalid");
        static_assert(validate_arguments<TArguments>(), "Arguments type is invalid");

    protected:
        template<typename TFrameOptions>
        using frame = command_frame<TFrameOptions>;

        using result = simple_parse_result<TOptions, TArguments>;

    public:
        template<typename TMapper, typename ... TPrevious>
        void invoke(const TParser& parser, const TMapper& mapper, int argc, const char* const* argv,
            TPrevious&& ... previous) {
            static_assert((std::is_rvalue_reference_v<TPrevious&&> && ...),
                "All previous parameters must be passed as rvalue-references");

            auto result = this->parse_cmd(parser, mapper, argc, argv);

            static_cast<TInherit*>(this)->run(std::move(previous) ..., std::move(result));
        }
    };

    template<typename TInherit, typename TOptions>
    class forwarding_command : public multi_command_dispatcher<TOptions> {
        template<typename TMapper, typename ... TPrevious>
        struct command_bag_adapter {
        private:
            std::reference_wrapper<typename forwarding_command::template command_bag_t<TMapper, TPrevious ...>> cmds;

        public:
            explicit command_bag_adapter(
                std::reference_wrapper<typename forwarding_command::template command_bag_t<TMapper, TPrevious ...>> cmds)
                : cmds(cmds) {}

            template<typename TCommand>
            void add_command(std::string cmd_name, TCommand cmd) {
                cmds.get().emplace(std::move(cmd_name),
                    std::make_unique<typename forwarding_command::template
                        any_command_erased<TCommand, TMapper, TPrevious ...>>(std::move(cmd)));
            }
        };

    protected:
        template<typename ... TPrevious>
        struct params {};

        struct any_params {
            template<typename ... TPrevious>
            any_params(params<TPrevious ...>) {}
        };

        template<typename TFrameOptions>
        using frame = command_frame<TFrameOptions>;

    public:
        template<typename TMapper, typename ... TPrevious>
        void invoke(const typename forwarding_command::TParser& parser, const TMapper& mapper, int argc, const char* const* argv,
            TPrevious&& ... previous) {
            static_assert((std::is_rvalue_reference_v<TPrevious&&> && ...), "All previous types must be rvalue-references");

            typename forwarding_command::template command_bag_t<TMapper, TPrevious ..., command_frame<TOptions>> cmds;

            static_cast<TInherit*>(this)->commands(params<TPrevious ...>{},
                command_bag_adapter<TMapper, TPrevious ..., command_frame<TOptions>>{std::ref(cmds)});

            this->parse_cmd(std::move(cmds), parser, mapper, argc, argv, std::move(previous) ...);
        }
    };

}

#endif //CPPCMD_COMMAND_H