#ifndef CPPCMD_OPTIONS_DEFINITION_H
#define CPPCMD_OPTIONS_DEFINITION_H

#include <optional>
#include <type_traits>
#include <utility>

#include "config.h"
#include "arguments.h"
#include "validators.h"

namespace cppcmd {

    namespace detail {

        template<typename T, typename TParser>
        class option {
            using config_t = config::detail::parameter_pack<typename TParser::option_config,
                    config::default_value<T>,
                    config::implicit_value<T>,
                    config::description,
                    config::short_name,
                    config::validator_storage<T>,
                    config::long_name>;

        public:
            std::optional<T> val;
            std::optional<typename TParser::option_config> parser_config;
            std::optional<config::default_value<T>> value_no_opt;
            std::optional<config::implicit_value<T>> value_no_arg;
            std::optional<config::description> description;
            std::optional<config::long_name> long_name;
            std::optional<config::short_name> short_name;
            std::optional<config::validator_storage<T>> validators;

            bool requires_arg = !std::is_same_v<bool, T>;

            using type = T;

            template<typename M = T,
                    std::enable_if_t<!std::is_same_v<bool, M>, int> = 0>
            option() {}

            template<typename M = T,
                    std::enable_if_t<std::is_same_v<bool, M>, int> = 0>
            option()
                    : value_no_opt(config::default_value{false}),
                      value_no_arg(config::implicit_value{true}) {}

            template<typename ... Args, typename M = T,
                    std::enable_if_t<(config::detail::is_in_tuple_v<config_t, Args> && ...), int> = 0,
                    std::enable_if_t<!std::is_same_v<bool, M>, int> = 0>
            explicit option(Args&& ... args)
                    : parser_config(config::from_args_opt<typename TParser::option_config>(std::forward<Args>(args) ...)),
                      value_no_opt(config::from_args_opt<config::default_value<T>>(std::forward<Args>(args) ...)),
                      value_no_arg(config::from_args_opt<config::implicit_value<T>>(std::forward<Args>(args) ...)),
                      description(config::from_args_opt<config::description>(std::forward<Args>(args) ...)),
                      long_name(config::from_args_opt<config::long_name>(std::forward<Args>(args) ...)),
                      short_name(config::from_args_opt<config::short_name>(std::forward<Args>(args) ...)),
                      validators(config::from_args_opt<config::validator_storage<T>>(std::forward<Args>(args) ...)) {}

            template<typename ... Args, typename M = T,
                    std::enable_if_t<(config::detail::is_in_tuple_v<config_t, Args> && ...), int> = 0,
                    std::enable_if_t<std::is_same_v<bool, M>, int> = 0>
            explicit option(Args&& ... args)
                    : parser_config(config::from_args_opt<typename TParser::option_config>(std::forward<Args>(args) ...)),
                      value_no_opt(config::from_args(config::default_value<T>{false}, std::forward<Args>(args) ...)),
                      value_no_arg(config::from_args(config::implicit_value<T>{true}, std::forward<Args>(args) ...)),
                      description(config::from_args_opt<config::description>(std::forward<Args>(args) ...)),
                      long_name(config::from_args_opt<config::long_name>(std::forward<Args>(args) ...)),
                      short_name(config::from_args_opt<config::short_name>(std::forward<Args>(args) ...)),
                      validators(config::from_args_opt<config::validator_storage<T>>(std::forward<Args>(args) ...)) {}

            void set_value(T new_value) {
                val = std::move(new_value);
            }

            const T& get_value() {
                return *(*this);
            }

            const T& operator*() {
                return val.value();
            }
        };

        template<typename T, typename TParser>
        class multioption {
            static_assert(config::detail::is_iterable_v<T>, "Multivalue type must be iterable");
            static_assert(std::is_default_constructible_v<T>, "Multivalue type must be default-constructible");
            static_assert(std::is_default_constructible_v<config::detail::iterated_type<T>>,
                          "Iterated type in multivalue must be default-constructible");

            using settings_t = config::detail::parameter_pack<typename TParser::option_config,
                    config::default_value<T>,
                    config::implicit_single_value<config::detail::iterated_type<T>>,
                    config::description,
                    config::short_name,
                    config::validator_storage<T>,
                    config::long_name>;

        public:
            std::optional<config::implicit_single_value<config::detail::iterated_type<T>>> value_no_single_arg;
            std::optional<T> val;
            std::optional<config::default_value<T>> value_no_opt;
            std::optional<typename TParser::option_config> parser_config;
            std::optional<config::description> description;
            std::optional<config::long_name> long_name;
            std::optional<config::short_name> short_name;
            std::optional<config::validator_storage<T>> validators;

            bool requires_arg = true;

            using type = T;

            using iterated_type = config::detail::iterated_type<T>;

            multioption() = default;

            template<typename ... Args, typename M = T,
                    std::enable_if_t<(config::detail::is_in_tuple_v<settings_t, Args> && ...), int> = 0,
                    std::enable_if_t<!std::is_same_v<bool, config::detail::iterated_type<M>>, int> = 0>
            explicit multioption(Args&& ... args)
                    : value_no_single_arg(config::from_args_opt<config::implicit_single_value<config::detail::iterated_type<M>>>(std::forward<Args>(args) ...)),
                      value_no_opt(config::from_args_opt<config::default_value<T>>(std::forward<Args>(args) ...)),
                      parser_config(config::from_args_opt<typename TParser::option_config>(std::forward<Args>(args) ...)),
                      description(config::from_args_opt<config::description>(std::forward<Args>(args) ...)),
                      long_name(config::from_args_opt<config::long_name>(std::forward<Args>(args) ...)),
                      short_name(config::from_args_opt<config::short_name>(std::forward<Args>(args) ...)),
                      validators(config::from_args_opt<config::validator_storage<T>>(std::forward<Args>(args) ...)) {}

            template<typename ... Args, typename M = T,
                    std::enable_if_t<(config::detail::is_in_tuple_v<settings_t, Args> && ...), int> = 0,
                    std::enable_if_t<std::is_same_v<bool, config::detail::iterated_type<M>>, int> = 0>
            explicit multioption(Args&& ... args)
                    : value_no_single_arg(config::from_args(config::implicit_single_value{true}, std::forward<Args>(args) ...)),
                      value_no_opt(config::from_args_opt<config::default_value<T>>(std::forward<Args>(args) ...)),
                      parser_config(config::from_args_opt<typename TParser::option_config>(std::forward<Args>(args) ...)),
                      description(config::from_args_opt<config::description>(std::forward<Args>(args) ...)),
                      long_name(config::from_args_opt<config::long_name>(std::forward<Args>(args) ...)),
                      short_name(config::from_args_opt<config::short_name>(std::forward<Args>(args) ...)),
                      validators(config::from_args_opt<config::validator_storage<T>>(std::forward<Args>(args) ...)) {}

            void set_value(T new_value) {
                val = std::move(new_value);
            }

            const T& get_value() {
                return *(*this);
            }

            const T& operator*() {
                return val.value();
            }
        };

        template<typename TParser>
        using flag = option<bool, TParser>;

        template<typename T = void>
        struct is_single_option : std::false_type {};

        template<typename T, typename TParser>
        struct is_single_option<option<T, TParser>> : std::true_type {};

        template<typename T>
        inline constexpr bool is_single_option_v = is_single_option<T>::value;

        template<typename T = void>
        struct is_multioption : std::false_type {};

        template<typename T, typename TParser>
        struct is_multioption<multioption<T, TParser>> : std::true_type {};

        template<typename T>
        inline constexpr bool is_multioption_v = is_multioption<T>::value;

        template<typename T>
        inline constexpr bool is_any_option_v = is_single_option_v<T> || is_multioption_v<T>;

        template<typename T, typename = void>
        struct option_type {
            using type = T;
        };

        template<typename T>
        struct option_type<T, std::enable_if_t<is_any_option_v<T>>> {
            using type = typename T::type;
        };

        template<typename T>
        using option_type_t = typename option_type<T>::type;

        template<typename TField>
        std::string_view get_long_name(const TField& field) {
            using field_type = config::detail::field_t<TField>;

            std::string_view long_name;

            if constexpr (is_any_option_v<field_type> || is_any_argument_v<field_type>) {
                const auto& value = field.value();

                auto field_name = field.name();

                const std::optional<config::long_name>* value_long_name;

                if constexpr (is_any_option_v<field_type>) {
                    value_long_name = &value->long_name;
                } else {
                    value_long_name = &value->name;
                }

                long_name = value_long_name->has_value() ? value_long_name->value().value.c_str() : field_name;
            } else {
                long_name = field.name();
            }

            return long_name;
        }

        template<typename TField>
        std::optional<char> get_short_name(const TField& field) {
            using option_type = config::detail::field_t<TField>;

            if constexpr (is_any_option_v<option_type>) {
                const auto& value = field.value();

                auto& value_short_name = value->short_name;

                if (value_short_name.has_value()) {
                    return value_short_name->value;
                }
            }

            return std::nullopt;
        }

        template<typename TField>
        bool has_default_value(const TField& field) {
            using option_type = config::detail::field_t<TField>;

            if constexpr (is_any_option_v<option_type>) {
                const auto& value = field.value();

                const auto& default_value = value->value_no_opt;

                return default_value.has_value();
            }

            return false;
        }

        template<typename TField>
        bool has_implicit_value(const TField& field) {
            using option_type = config::detail::field_t<TField>;

            if constexpr (is_single_option_v<option_type>) {
                const auto& value = field.value();

                const auto& implicit_value = value->value_no_arg;

                return implicit_value.has_value();
            }

            return false;
        }

        template<typename TField>
        bool has_implicit_single_value(const TField& field) {
            using option_type = config::detail::field_t<TField>;

            if constexpr (is_multioption_v<option_type>) {
                const auto& value = field.value();

                const auto& implicit_value = value->value_no_single_arg;

                return implicit_value.has_value();
            }

            return false;
        }

        template<typename TField>
        auto& get_value_ref(TField& field) {
            using field_type = config::detail::field_t<TField>;

            if constexpr (is_any_option_v<field_type> || is_any_argument_v<field_type>) {
                return field.value()->val;
            } else {
                return *field.value();
            }
        }

        template<typename TField>
        auto get_default_value(const TField& field) {
            static_assert(is_any_option_v<config::detail::field_t<TField>>,
                          "Only cppcmd::value or cppcmd::multivalue type can provide default value");

            return field.value()->value_no_opt->value();
        }

        template<typename TField>
        auto get_implicit_value(const TField& field) {
            static_assert(is_single_option_v<config::detail::field_t<TField>>,
                          "Only cppcmd::value type can provide implicit value");

            return field.value()->value_no_arg->value();
        }

        template<typename TField>
        auto get_implicit_single_value(const TField& field) {
            static_assert(is_multioption_v<config::detail::field_t<TField>>,
                          "Only cppcmd::multivalue type can provide implicit value for single argument");

            return field.value()->value_no_single_arg->value();
        }

        template<typename TField>
        bool option_requires_arg(const TField& field) {
            using option_type = config::detail::field_t<TField>;

            if constexpr (is_any_option_v<option_type>) {
                return field.value()->requires_arg;
            }

            return !std::is_same_v<option_type, bool>;
        }

    }

    template<typename TParser>
    class basic_options {
    protected:
        template<typename T>
        using option = detail::option<T, TParser>;

        template<typename T>
        using multioption = detail::multioption<T, TParser>;

        using flag = detail::flag<TParser>;

    public:
        using parser_t = TParser;
    };

    template<typename TOptions>
    struct is_options {
        template<typename TParser>
        static std::true_type test(basic_options<TParser>*);

        static std::false_type test(void*);

        using type = decltype(test(std::declval<std::decay_t<TOptions>*>()));
    };

    template<typename TOptions>
    inline constexpr bool is_options_v = is_options<TOptions>::type::value;

}

#endif //CPPCMD_OPTIONS_DEFINITION_H