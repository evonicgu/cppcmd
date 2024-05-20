#ifndef CPPCMD_DEFAULT_VALUE_PARSER_H
#define CPPCMD_DEFAULT_VALUE_PARSER_H

#include <charconv>
#include <string>
#include <type_traits>

#include "cppcmd/exception.h"
#include "cppcmd/config.h"

namespace cppcmd::parser {

    struct default_value_parser_config {
        char value_separator = ',';
    };

    class default_value_parser {
        default_value_parser_config config;

    public:
        explicit default_value_parser(default_value_parser_config config)
            : config(config) {}

        default_value_parser() = default;

        template<typename TNumeric,
            typename = std::enable_if_t<std::is_integral_v<TNumeric> || std::is_floating_point_v<TNumeric>>>
        void parse(std::string_view text, TNumeric& value) const {
            std::from_chars_result result = std::from_chars(text.data(), text.data() + text.size(), value);

            if (result.ec == std::errc::result_out_of_range) {
                throw exception::parsing::unable_to_parse_value(
                    "Numeric value out of range");
            }
            if (result.ec != std::errc{}) {
                throw exception::parsing::unable_to_parse_value(
                    "Could not parse numeric value");
            }

            if (result.ptr != text.data() + text.size()) {
                throw exception::parsing::unable_to_parse_value(
                    "Invalid characters encountered, could not parse numeric value");
            }
        }

        void parse(std::string_view text, std::string& value) const {
            value = text;
        }

        template<typename TContainer, std::enable_if_t<config::detail::is_iterable_v<TContainer>, int> = 0>
        void parse(std::string_view text, TContainer& value) const {
            std::size_t prev_pos = 0, curr_pos = text.find(config.value_separator);

            using iterated_type = config::detail::iterated_type<TContainer>;

            while (prev_pos != std::string_view::npos) {
                std::string_view curr_value = text.substr(prev_pos,
                    curr_pos == std::string_view::npos ? curr_pos : curr_pos - prev_pos);

                iterated_type parsed;
                parse(curr_value, parsed);

                value.insert(value.end(), std::move(parsed));

                prev_pos = curr_pos;

                if (curr_pos != std::string_view::npos) {
                    ++prev_pos;
                }

                curr_pos = text.find(config.value_separator, prev_pos);
            }
        }

        void parse(std::string_view text, char& value) const {
            if (text.size() != 1) {
                throw exception::parsing::unable_to_parse_value("Character value too long");
            }

            value = text[0];
        }

        template<typename T>
        void parse(std::string_view text, std::optional<T>& value) const {
            if (text.empty()) {
                value = std::nullopt;
                return;
            }

            T inner;

            parse(text, inner);

            value = std::move(inner);
        }

        void parse(std::string_view text, bool& value) const {
            if (text.size() == 1) {
                switch (text[0]) {
                    case 'T':
                    case 't':
                    case '1':
                        value = true;
                        break;
                    case 'F':
                    case 'f':
                    case '0':
                        value = false;
                        break;
                    default:
                        throw exception::parsing::unable_to_parse_value("Could not parse boolean value");
                }

                return;
            }

            if (text.size() == 4) {
                if ((text[0] == 'T' || text[0] == 't') && text[1] == 'r' && text[2] == 'u' && text[3] == 'e') {
                    value = true;
                    return;
                }

                throw exception::parsing::unable_to_parse_value("Could not parse boolean value");
            }

            if (text.size() == 5) {
                if ((text[0] == 'F' || text[0] == 'f') && text[1] == 'a' && text[2] == 'l' && text[3] == 's' && text[4] == 'e') {
                    value = false;
                    return;
                }
            }

            throw exception::parsing::unable_to_parse_value("Could not parse boolean value");
        }
    };

}

#endif //CPPCMD_DEFAULT_VALUE_PARSER_H