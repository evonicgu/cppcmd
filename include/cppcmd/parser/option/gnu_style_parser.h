#ifndef CPPCMD_GNU_STYLE_PARSER_H
#define CPPCMD_GNU_STYLE_PARSER_H

#include <algorithm>
#include <string>
#include <unordered_map>
#include <rfl/to_view.hpp>

#include "parsing.h"
#include "cppcmd/exception.h"
#include "cppcmd/options.h"

namespace cppcmd::parser {

    class gnu_style_parser {
    public:
        struct options_prototype_t {
            std::unordered_map<std::string, bool> long_options;
            std::unordered_map<char, bool> short_options;
        };

        struct empty_config {};

        using option_config = empty_config;
        using argument_config = option_config;

        gnu_style_parser() = default;

        template<typename TOptions>
        options_prototype_t create_prototype(const TOptions& options) const {
            options_prototype_t out;

            const auto view = rfl::to_view(options);

            view.apply([&](const auto& field) {
                std::string_view long_name = detail::get_long_name(field);
                std::string_view field_name = field.name();
                std::optional<char> short_name = detail::get_short_name(field);
                bool requires_arg = detail::option_requires_arg(field);

                // deliberately not checking for content of the option names, this check should be done in parser
                if (!is_long_option_name_valid(long_name)) {
                    throw exception::specification::long_option_invalid_name(
                        "Long option at field '" + std::string{field_name} + "' has invalid name: " + std::string{long_name});
                }

                auto inserted = out.long_options.emplace(long_name, requires_arg).second;

                if (!inserted) {
                    throw exception::specification::duplicate_long_option_name(
                        "Long option at field '" + std::string{field_name} + "' has been already declared: " + std::string{long_name});
                }

                if (short_name.has_value()) {
                    if (!isalpha(short_name.value())) {
                        throw exception::specification::short_option_invalid_name(
                            "Short option at field '" + std::string{field_name} + "' is invalid: " + short_name.value());
                    }

                    inserted = out.short_options.emplace(short_name.value(), requires_arg).second;

                    if (!inserted) {
                        throw exception::specification::duplicate_short_option_name(
                            "Short option at field '" + std::string{field_name} + "' has been already declared: " + short_name.value());
                    }
                }
            });

            return out;
        }

        bool validate_cmd_name(const std::string& cmd_name) const {
            if (cmd_name.empty()) {
                return false;
            }

            if (cmd_name[0] == '-') {
                return false;
            }

            auto valid_char = [](char c, bool allow_digits = true) {
                return isalpha(c) || (isdigit(c) && allow_digits) || c == '$' || c == '_' || c == '-';
            };

            return valid_char(cmd_name[0], false) &&
                std::all_of(cmd_name.begin(), cmd_name.end(), valid_char);
        }

        parsing_result parse(int argc, const char* const* argv, const options_prototype_t& prototype,
            argument_limit limit) const {
            parsing_result out{};

            bool in_args = false, in_option = false;
            std::size_t option_argument_counter = 0;

            std::optional<std::string>* current_option_arg = nullptr;
            std::vector<std::pair<std::string, std::size_t>>* current_option_args = nullptr;

            for (int i = 0; i < argc; ++i) {
                if (limit == argument_limit::single && !out.arguments.empty()) {
                    return out;
                }

                ++out.args_used;

                std::string curr = argv[i];

                if (in_args) {
                    out.arguments.push_back(std::move(curr));
                    continue;
                }

                bool curr_is_option = is_option(curr);

                if (!curr_is_option) {
                    if (!in_option) {
                        out.arguments.push_back(std::move(curr));
                    } else {
                        *current_option_arg = std::move(curr);
                        in_option = false;
                    }

                    continue;
                }

                in_option = false;

                if (is_args_specifier(curr)) {
                    if (limit == argument_limit::single) {
                        throw exception::parsing::parsing_exception("Cannot use '--' specifier in the middle of the command invocation");
                    }

                    in_args = true;
                    continue;
                }

                // currently in option
                if (is_short_option(curr)) {
                    for (std::size_t j = 1; j < curr.size(); ++j) {
                        char c = curr[j];

                        if (!isalpha(c)) {
                            throw exception::parsing::short_option_invalid_name(
                                std::string("Short option name '") + c + "' is invalid");
                        }

                        auto proto_it = prototype.short_options.find(c);

                        auto short_opt_it = out.short_options.insert({c, {}}).first;
                        short_opt_it->second.emplace_back(std::nullopt, option_argument_counter++);

                        if (proto_it != prototype.short_options.end() && proto_it->second && j == curr.size() - 1) {
                            // option is present, is last and is not a flag
                            current_option_arg = &short_opt_it->second.back().first;
                            in_option = true;
                        }
                    }
                } else {
                    auto eq_idx = curr.find('=');

                    if (curr.size() < 4 || eq_idx < 4) {
                        throw exception::parsing::long_option_too_short(
                            "Long option name '" + curr.substr(2, eq_idx) +
                            "' is too short. Must be at least 2 characters long");
                    }

                    std::optional<std::string> opt_value;

                    if (eq_idx != std::string::npos) {
                        opt_value = curr.substr(eq_idx + 1);
                    }

                    auto opt_name = curr.substr(2, eq_idx - 2);

                    if (!is_long_option_name_valid(opt_name)) {
                        throw exception::parsing::long_option_invalid_name(
                            "Long option name '" + opt_name + "' is invalid");
                    }

                    auto proto_it = prototype.long_options.find(opt_name);

                    auto long_opt_it = out.long_options.emplace(
                        std::move(opt_name), parsing_result::invocations_t{}).first;

                    bool has_value = opt_value.has_value();

                    long_opt_it->second.emplace_back(std::move(opt_value), option_argument_counter++);

                    if (proto_it != prototype.long_options.end() && proto_it->second && !has_value) {
                        current_option_arg = &long_opt_it->second.back().first;
                        in_option = true;
                    }
                }
            }

            return out;
        }

    private:
        static bool is_option(const std::string& name) {
            return name.size() > 1 && name[0] == '-';
        }

        static bool is_args_specifier(const std::string& name) {
            return name.size() == 2 && name[0] == '-' && name[1] == '-';
        }

        static bool is_short_option(const std::string& name) {
            return name[1] != '-';
        }

        static bool is_long_option_name_valid(std::string_view name) {
            if (name.size() < 2 || (!isalpha(name[0]) && name[0] != '$' && name[0] != '_')) {
                return false;
            }

            return std::all_of(name.begin(), name.end(), [](char c) {
                return isalnum(c) || c == '-' || c == '$' || c == '_';
            });
        }
    };

}

#endif //CPPCMD_GNU_STYLE_PARSER_H