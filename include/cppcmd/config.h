#ifndef CPPCMD_CONFIG_H
#define CPPCMD_CONFIG_H

#include <optional>
#include <type_traits>
#include <string>

namespace cppcmd::config {

    namespace detail {

        template<typename ... T>
        struct parameter_pack {};

        template<typename T>
        struct is_optional : public std::false_type {};

        template<typename T>
        struct is_optional<std::optional<T>> : public std::true_type {};

        template<typename T>
        inline constexpr bool is_optional_v = is_optional<T>::value;

        template<typename T, typename = void>
        struct is_iterable : public std::false_type {};

        template<typename T>
        struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())),
            decltype(std::end(std::declval<T>())), typename T::iterator>> : public std::true_type {};

        template<typename T>
        inline constexpr bool is_iterable_v = is_iterable<T>::value;

        template<typename T, std::enable_if_t<is_iterable_v<T>, int> = 0>
        using iterated_type = typename std::iterator_traits<typename T::iterator>::value_type;

        template<typename , typename , typename ...>
        class is_in_params_impl : public std::false_type {};

        template<typename Tp, typename ... Args>
        class is_in_params_impl<Tp, std::enable_if_t<(std::is_same_v<std::decay_t<Tp>, std::decay_t<Args>> || ...) ||
            (std::is_convertible_v<std::decay_t<Tp>, std::decay_t<Args>> || ...)>, Args ...> : public std::true_type {};

        template<typename Tp, typename ... Args>
        using is_in_params = is_in_params_impl<Tp, void, Args ...>;

        template<typename Tp, typename ... Args>
        inline constexpr bool is_in_params_v = is_in_params<Tp, Args ...>::value;

        template<typename Tuple, typename Tp, typename V = void>
        class is_in_tuple : public std::false_type {};

        template<typename Tp, typename ... TupleArgs>
        class is_in_tuple<parameter_pack<TupleArgs ...>, Tp,
            std::enable_if_t<is_in_params_v<Tp, TupleArgs ...>>> : public std::true_type {};

        template<typename Tuple, typename Tp>
        inline constexpr bool is_in_tuple_v = is_in_tuple<Tuple, Tp>::value;

        template<typename TField>
        using field_t = std::decay_t<std::remove_pointer_t<typename TField::Type>>;

    }

    template<typename T>
    struct default_value {
        T value;

        explicit default_value(T value)
            : value(std::move(value)) {}

        template<typename Tp, std::enable_if_t<std::is_convertible_v<Tp, T>, int> = 0>
        default_value(default_value<Tp> other)
            : value(std::move(other.value)) {}
    };

    template<typename T>
    struct implicit_value {
        T value;

        explicit implicit_value(T value)
            : value(std::move(value)) {}

        template<typename Tp, std::enable_if_t<std::is_convertible_v<Tp, T>, int> = 0>
        implicit_value(implicit_value<Tp> other)
            : value(std::move(other.value)) {}
    };

    template<typename T>
    struct implicit_single_value {
        T value;

        explicit implicit_single_value(T value)
            : value(std::move(value)) {}

        template<typename Tp, std::enable_if_t<std::is_convertible_v<Tp, T>, int> = 0>
        implicit_single_value(implicit_single_value<Tp> other)
            : value(std::move(other.value)) {}
    };

    struct description {
        std::string value;

        explicit description(std::string value)
            : value(std::move(value)) {}
    };

    struct short_name {
        char value;

        explicit short_name(char value)
            : value(value) {}
    };

    struct long_name {
        std::string value;

        explicit long_name(std::string value)
            : value(std::move(value)) {}
    };

    template<typename T, typename Arg>
    T&& from_args(T&& def, Arg&& arg) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<Arg>>) {
            return std::forward<Arg>(arg);
        }

        return std::forward<T>(def);
    }

    template<typename T, typename Arg, typename ... Args>
    T&& from_args(T&& def, Arg&& arg, Args&& ... args) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<Arg>>) {
            return std::forward<Arg>(arg);
        }

        return from_args(std::forward<T>(def), std::forward<Args>(args) ...);
    }

    template<typename T, typename Arg>
    std::optional<T> from_args_opt(Arg&& arg) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<Arg>> ||
            std::is_convertible_v<std::decay_t<Arg>, std::decay_t<T>>) {
            return std::forward<Arg>(arg);
        }

        return std::nullopt;
    }

    template<typename T, typename Arg, typename ... Args>
    std::optional<T> from_args_opt(Arg&& arg, Args&& ... args) {
        if constexpr (std::is_same_v<std::decay_t<T>, std::decay_t<Arg>> ||
            std::is_convertible_v<std::decay_t<Arg>, std::decay_t<T>>) {
            return std::forward<Arg>(arg);
        }

        return from_args_opt<T, Args ...>(std::forward<Args>(args) ...);
    }

    template<typename T, typename ... Args>
    T from_args_guaranteed(Args&& ... args) {
        return from_args_opt<T, Args ...>(std::forward<Args>(args) ...).value();
    }

}

#endif //CPPCMD_CONFIG_H
