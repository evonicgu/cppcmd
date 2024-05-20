#ifndef CPPCMD_DEFAULT_MAPPER_H
#define CPPCMD_DEFAULT_MAPPER_H

#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <unordered_map>

#include "rfl.hpp"

#include "cppcmd/options.h"
#include "cppcmd/arguments.h"
#include "cppcmd/exception.h"
#include "cppcmd/config.h"
#include "cppcmd/parser/option/parsing.h"

namespace cppcmd {

    struct default_mapper_configuration {
        bool allow_unrecognized_options = false;
        bool allow_too_many_passed = false;
        bool allow_excessive_arguments = false;
    };

    struct unmatched_data {
        using unmatched_options_t =
            std::unordered_map<std::string, std::vector<std::optional<std::string>>>;

        unmatched_options_t unmatched_options;
        std::vector<std::string> unmatched_arguments;
    };

    template<typename TValueParser>
    class default_mapper {
        default_mapper_configuration config{};
        TValueParser value_parser;

    public:
        explicit default_mapper(default_mapper_configuration config, TValueParser value_parser)
            : config(config),
              value_parser(std::move(value_parser)) {}

        explicit default_mapper(TValueParser value_parser)
            : value_parser(std::move(value_parser)) {}

        template<typename TOptions, typename TArguments>
        unmatched_data map(TOptions& options, TArguments& args, parser::parsing_result& parsed) const {
            unmatched_data out;

            map_options<TOptions>(options, parsed, out);

            map_arguments<TArguments>(args, parsed, out);

            return out;
        }

    private:
        template<typename TOptions>
        void map_options(TOptions& options, parser::parsing_result& parsed, unmatched_data& unmatched) const {
            auto view = rfl::to_view(options);

            view.apply([&](auto field) {
                using field_type = config::detail::field_t<decltype(field)>;
                auto& value = *field.value();

                // convert to string since it will be implicitly converted during search anyway
                std::string long_name{detail::get_long_name(field)};
                std::optional<char> short_name = detail::get_short_name(field);

                auto long_args_it = parsed.long_options.find(long_name);
                auto short_args_it = short_name.has_value() ? parsed.short_options.find(short_name.value()) : parsed.short_options.end();

                // no arguments specified for both long and short options
                if (long_args_it == parsed.long_options.end() && short_args_it == parsed.short_options.end()) {
                    if constexpr (config::detail::is_optional_v<field_type>) {
                        detail::get_value_ref(field) = std::nullopt;
                        return;
                    } else if constexpr (std::is_same_v<field_type, bool>) {
                        detail::get_value_ref(field) = false;
                        return;
                    }

                    if (!detail::has_default_value(field)) {
                        throw exception::parsing::option_value_missing(
                            "Option '" + long_name + "' has no default value and no value was provided");
                    }

                    // the previous check would fail if not any option, but this check is needed to be type safe
                    if constexpr (detail::is_any_option_v<field_type>) {
                        value.set_value(detail::get_default_value(field));
                    }

                    return;
                }

                auto option_args = collect_and_erase(parsed, long_args_it, short_args_it);

                map_single_option(field, option_args);

                if constexpr (detail::is_any_option_v<field_type>) {

                    if (!value.validators.has_value()) {
                        return;
                    }

                    auto err = value.validators->validate(value.get_value());

                    if (!err.has_value()) {
                        return;
                    }

                    throw exception::parsing::validation_error("Failed to validate option '" +
                        long_name + "': " + err.value());
                }
            });

            if (!config.allow_unrecognized_options && (!parsed.long_options.empty() || !parsed.short_options.empty())) {
                const std::string& option_name = parsed.long_options.empty() ?
                    std::string{parsed.short_options.begin()->first} :
                    parsed.long_options.begin()->first;

                throw exception::parsing::unrecognized_option_name("Invalid option name '" +
                     option_name + "'");
            }

            copy_unmatched_options(unmatched, parsed);
        }

        template<typename TArguments>
        void map_arguments(TArguments& args, parser::parsing_result& parsed, unmatched_data& unmatched) const {
            std::size_t current_arg = 0;

            auto view = rfl::to_view(args);

            view.apply([&](auto field) {
                using arg_type = config::detail::field_t<decltype(field)>;

                if (detail::is_arg_required<arg_type>() && current_arg == parsed.arguments.size()) {
                    throw exception::parsing::argument_value_missing(
                        "Positional argument '" + std::string{detail::get_long_name(field)} +
                        "' is mandatory and no value was provided");
                }

                if constexpr (detail::is_argument_sink_v<arg_type>) {
                    detail::get_value_ref(field).emplace();

                    std::transform(parsed.arguments.begin() + current_arg, parsed.arguments.end(),
                        std::inserter(detail::get_value_ref(field).value(), detail::get_value_ref(field)->end()),
                        [&](const std::string& arg) {
                            typename arg_type::iterated_type val;

                            value_parser.parse(arg, val);
                            ++current_arg;

                            return std::move(val);
                        });
                } else if constexpr (detail::is_single_argument_v<arg_type> && detail::is_arg_required<arg_type>()) {
                    typename arg_type::type val;

                    value_parser.parse(parsed.arguments[current_arg++], val);

                    get_value_ref(field) = val;
                } else if constexpr (!detail::is_arg_required<arg_type>()) {
                    if (parsed.arguments.size() == current_arg) {
                        detail::get_value_ref(field) = field.value()->value_no_arg.value;
                    } else {
                        typename arg_type::type val;

                        value_parser.parse(parsed.arguments[current_arg++], val);

                        detail::get_value_ref(field) = val;
                    }
                } else {
                    value_parser.parse(parsed.arguments[current_arg++], *field.value());
                }

                if constexpr (detail::is_any_argument_v<arg_type>) {
                    auto& validators = field.value()->validators;

                    if (!validators.has_value()) {
                        return;
                    }

                    const err_t err = validators->validate(field.value()->get_value());

                    if (!err.has_value()) {
                        return;
                    }

                    throw exception::parsing::validation_error("Failed to validate option '" +
                        std::string{detail::get_long_name(field)} + "': " + err.value());
                }
            });

            if (current_arg != parsed.arguments.size() && !config.allow_excessive_arguments) {
                throw exception::parsing::excessive_arguments("Too many arguments given");
            }

            for (auto i = current_arg; i < parsed.arguments.size(); ++i) {
                unmatched.unmatched_arguments.push_back(std::move(parsed.arguments[i]));
            }
        }

        template<typename TField>
        void map_single_option(TField& field, std::vector<std::optional<std::string>>& arguments) const {
            using option_type = config::detail::field_t<TField>;

            if (!detail::is_multioption_v<option_type> && arguments.size() > 1 && !config.allow_too_many_passed) {
                throw exception::parsing::too_many_values(
                    "Expected option '" + std::string{detail::get_long_name(field)} + "' to have no more than 1 value");
            }

            if constexpr (detail::is_multioption_v<option_type>) {
                detail::get_value_ref(field).emplace();

                std::transform(arguments.begin(), arguments.end(),
                    std::inserter(detail::get_value_ref(field).value(), detail::get_value_ref(field)->end()),
                    [&](const std::optional<std::string>& arg) {
                        if (!arg.has_value()) {
                            if (!detail::has_implicit_single_value(field)) {
                                throw exception::parsing::no_implicit_single_value(
                                    "No implicit single value exists for option '" + std::string{detail::get_long_name(field)} +
                                    "' and no value was given in one of the usages");
                            }

                            return detail::get_implicit_single_value(field);
                        }

                        typename option_type::iterated_type val;

                        value_parser.parse(arg.value(), val);

                        return std::move(val);
                    });
            } else {
                // single option
                if (arguments.back().has_value()) {
                    detail::option_type_t<option_type> val;
                    value_parser.parse(arguments.back().value(), val);

                    detail::get_value_ref(field) = std::move(val);
                    return;
                }

                if constexpr (detail::is_any_option_v<option_type>) {
                    if (detail::has_implicit_value(field)) {
                        detail::get_value_ref(field) = detail::get_implicit_value(field);
                        return;
                    }
                } else if constexpr (config::detail::is_optional_v<option_type>) {
                    detail::get_value_ref(field) = std::nullopt;
                    return;
                } else if constexpr (std::is_same_v<option_type, bool>) {
                    detail::get_value_ref(field) = true;
                    return;
                }

                throw exception::parsing::no_implicit_value(
                    "No implicit value exists for option '" + std::string{detail::get_long_name(field)} +
                    "' and no value was given");
            }
        }

        static std::vector<std::optional<std::string>> collect_and_erase(parser::parsing_result& parsed,
            parser::parsing_result::long_options_t::iterator long_args_it,
            parser::parsing_result::short_options_t::iterator short_args_it) {
            std::vector<std::pair<std::optional<std::string>, std::size_t>> empty;

            auto option_args = collect_option_arguments(
                long_args_it == parsed.long_options.end() ? empty : long_args_it->second,
                short_args_it == parsed.short_options.end() ? empty : short_args_it->second);

            if (long_args_it != parsed.long_options.end()) {
                parsed.long_options.erase(long_args_it);
            }

            if (short_args_it != parsed.short_options.end()) {
                parsed.short_options.erase(short_args_it);
            }

            return option_args;
        }

        static std::vector<std::optional<std::string>> collect_option_arguments(
            std::vector<std::pair<std::optional<std::string>, std::size_t>>& long_option_args,
            std::vector<std::pair<std::optional<std::string>, std::size_t>>& short_option_args) {

            std::vector<std::optional<std::string>> out;
            out.reserve(long_option_args.size() + short_option_args.size());

            std::size_t i = 0, j = 0;

            while (i < long_option_args.size() && j < short_option_args.size()) {
                if (long_option_args[i].second < short_option_args[j].second) {
                    out.push_back(std::move(long_option_args[i].first));
                    ++i;
                } else {
                    out.push_back(std::move(short_option_args[j].first));
                    ++j;
                }
            }

            for (std::size_t first = i; first < long_option_args.size(); ++first) {
                out.push_back(std::move(long_option_args[first].first));
            }

            for (std::size_t second = j; second < short_option_args.size(); ++second) {
                out.push_back(std::move(short_option_args[second].first));
            }

            return out;
        }

        static void copy_unmatched_options(unmatched_data& unmatched, parser::parsing_result& parsed) {
            for (auto it = parsed.long_options.begin(); it != parsed.long_options.end();) {
                auto tmp = it;
                ++it;

                auto nh = parsed.long_options.extract(tmp);

                std::vector<std::optional<std::string>> args;
                args.reserve(nh.mapped().size());

                for (auto& [str, pos] : nh.mapped()) {
                    args.push_back(std::move(str));
                }

                unmatched.unmatched_options.insert({std::move(nh.key()), std::move(args)});
            }

            for (auto it = parsed.short_options.begin(); it != parsed.short_options.end();) {
                auto tmp = it;
                ++it;

                auto nh = parsed.short_options.extract(tmp);

                std::vector<std::optional<std::string>> args;
                args.reserve(nh.mapped().size());

                for (auto& [str, pos] : nh.mapped()) {
                    args.push_back(std::move(str));
                }

                unmatched.unmatched_options.insert({std::string{nh.key()}, std::move(args)});
            }
        }
    };

}

#endif //CPPCMD_DEFAULT_MAPPER_H