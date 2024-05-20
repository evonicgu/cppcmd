#ifndef CPPCMD_ARGUMENTS_DEFINITION_H
#define CPPCMD_ARGUMENTS_DEFINITION_H

#include <optional>
#include <type_traits>
#include <utility>

#include "config.h"
#include "validators.h"

namespace cppcmd {

    namespace detail {

        template<typename T, typename TParser>
        class argument {
            using config_t = config::detail::parameter_pack<typename TParser::argument_config,
                config::description, config::long_name, config::validators<T>>;

        public:
            std::optional<T> val;
            std::optional<typename TParser::argument_config> parser_config;
            std::optional<config::description> description;
            std::optional<config::long_name> name;
            std::optional<config::validators<T>> validators;

            using type = T;

            static constexpr bool is_required = true;

            argument() = default;

            template<typename ... Args,
                    std::enable_if_t<(config::detail::is_in_tuple_v<config_t, Args> && ...), int> = 0>
            explicit argument(Args&& ... args)
                    : parser_config(config::from_args_opt<typename TParser::argument_config>(std::forward<Args>(args) ...)),
                      description(config::from_args_opt<config::description>(std::forward<Args>(args) ...)),
                      name(config::from_args_opt<config::long_name>(std::forward<Args>(args) ...)),
                      validators(config::from_args_opt<config::validators<T>>(std::forward<Args>(args) ...)) {}

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

        template<typename , typename , typename ...>
        class is_in_params_impl_reverse_convertible : public std::false_type {};

        template<typename Tp, typename ... Args>
        class is_in_params_impl_reverse_convertible<Tp, std::enable_if_t<(std::is_same_v<std::decay_t<Tp>, std::decay_t<Args>> || ...) || (std::is_convertible_v<std::decay_t<Args>, std::decay_t<Tp>> || ...)>, Args ...> : public std::true_type {};

        template<typename Tp, typename ... Args>
        using is_in_params = is_in_params_impl_reverse_convertible<Tp, void, Args ...>;

        template<typename Tp, typename ... Args>
        inline constexpr bool is_in_params_reverse_convertible_v = is_in_params<Tp, Args ...>::value;

        template<typename T, typename TParser>
        class optional_argument {
            using config_t = config::detail::parameter_pack<typename TParser::argument_config,
                config::description, config::long_name, config::default_value<T>, config::validators<T>>;

        public:
            std::optional<T> val;
            config::default_value<T> value_no_arg;
            std::optional<typename TParser::argument_config> parser_config;
            std::optional<config::description> description;
            std::optional<config::long_name> name;
            std::optional<config::validators<T>> validators;

            using type = T;

            static constexpr bool is_required = false;

            optional_argument() = delete;

            template<typename ... Args,
                    std::enable_if_t<(config::detail::is_in_tuple_v<config_t, Args> && ...), int> = 0,
                    std::enable_if_t<is_in_params_reverse_convertible_v<config::default_value<T>, Args ...>, int> = 0>
            explicit optional_argument(Args&& ... args)
                    : value_no_arg(config::from_args_guaranteed<config::default_value<T>>(std::forward<Args>(args) ...)),
                      parser_config(config::from_args_opt<typename TParser::argument_config>(std::forward<Args>(args) ...)),
                      description(config::from_args_opt<config::description>(std::forward<Args>(args) ...)),
                      name(config::from_args_opt<config::long_name>(std::forward<Args>(args) ...)),
                      validators(config::from_args_opt<config::validators<T>>(std::forward<Args>(args) ...)) {}

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
        class argument_sink {
            static_assert(config::detail::is_iterable_v<T>, "Argument sink type must be iterable");
            static_assert(std::is_default_constructible_v<config::detail::iterated_type<T>>,
                          "Iterated type in argument sink must be default-constructible");
            static_assert(std::is_default_constructible_v<T>, "Argument sink type must be default constructible");

            using config_t = config::detail::parameter_pack<typename TParser::argument_config,
                config::description, config::long_name, config::validators<T>>;

        public:
            std::optional<T> val;
            std::optional<typename TParser::argument_config> parser_config;
            std::optional<config::description> description;
            std::optional<config::long_name> name;
            std::optional<config::validators<T>> validators;

            using type = T;

            using iterated_type = config::detail::iterated_type<T>;

            static constexpr bool is_required = false;

            argument_sink() = default;

            template<typename ... Args,
                    std::enable_if_t<(config::detail::is_in_tuple_v<config_t, Args> && ...), int> = 0>
            explicit argument_sink(Args&& ... args)
                    : parser_config(config::from_args_opt<typename TParser::argument_config>(std::forward<Args>(args) ...)),
                      description(config::from_args_opt<config::description>(std::forward<Args>(args) ...)),
                      name(config::from_args_opt<config::long_name>(std::forward<Args>(args) ...)),
                      validators(config::from_args_opt<config::validators<T>>(std::forward<Args>(args) ...)) {}

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

        template<typename T = void>
        struct is_single_argument : std::false_type {};

        template<typename T, typename TParser>
        struct is_single_argument<argument<T, TParser>> : std::true_type {};

        template<typename T, typename TParser>
        struct is_single_argument<optional_argument<T, TParser>> : std::true_type {};

        template<typename T>
        inline constexpr bool is_single_argument_v = is_single_argument<T>::value;

        template<typename T = void>
        struct is_argument_sink : std::false_type {};

        template<typename T, typename TParser>
        struct is_argument_sink<argument_sink<T, TParser>> : std::true_type {};

        template<typename T>
        inline constexpr bool is_argument_sink_v = is_argument_sink<T>::value;

        template<typename T>
        inline constexpr bool is_any_argument_v = is_single_argument_v<T> || is_argument_sink_v<T>;

        template<typename TArg>
        constexpr bool is_arg_required() {
            if constexpr (is_any_argument_v<TArg>) {
                return TArg::is_required;
            }

            return true;
        }

        template<typename TField>
        constexpr std::size_t get_arg_strictness() {
            using argument_type = config::detail::field_t<TField>;

            if constexpr (is_single_argument_v<argument_type>) {
                return argument_type::is_required ? 2 : 1;
            } else if constexpr (is_argument_sink_v<argument_type>) {
                return 0;
            } else {
                return 2;
            }
        }

    }

    template<typename TParser>
    class basic_arguments {
    protected:
        template<typename T>
        using argument = detail::argument<T, TParser>;

        template<typename T>
        using optional_argument = detail::optional_argument<T, TParser>;

        template<typename T>
        using argument_sink = detail::argument_sink<T, TParser>;

    public:
        using parser_t = TParser;
    };

    template<typename TArguments>
    struct is_arguments {
        template<typename TParser>
        static std::true_type test(basic_arguments<TParser>*);

        static std::false_type test(void*);

        using type = decltype(test(std::declval<std::decay_t<TArguments>*>()));
    };

    template<typename TOptions>
    inline constexpr bool is_arguments_v = is_arguments<TOptions>::type::value;

}

#endif //CPPCMD_ARGUMENTS_DEFINITION_H
