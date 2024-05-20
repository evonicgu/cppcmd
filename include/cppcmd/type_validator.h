#ifndef CPPCMD_TYPE_VALIDATOR_H
#define CPPCMD_TYPE_VALIDATOR_H

#include <type_traits>

#include "rfl.hpp"

#include "cppcmd/options.h"
#include "cppcmd/arguments.h"

namespace cppcmd {

    template<typename>
    struct option_fields_validator : public std::false_type {};

    template<template<typename ...> typename NTuple, typename ... NTupleArgs>
    struct option_fields_validator<NTuple<NTupleArgs ...>> {
        static const bool value = ((!detail::is_any_argument_v<config::detail::field_t<NTupleArgs>>) && ...);
    };

    template<typename TOptions>
    consteval bool validate_options() {
        static_assert(std::is_default_constructible_v<TOptions>, "TOptions must be default-constructible");
        static_assert(is_options_v<TOptions>, "TOptions must inherit from cppcmd::options");

        static_assert(option_fields_validator<decltype(rfl::to_view(std::declval<TOptions&>()))>::value,
            "Option type is not allowed to have 'argument' members");

        return true;
    }

    template<typename , template <typename ...> typename, typename ...>
    struct extract_args;

    template<template <typename ... Args> typename TInherit, template <typename ... Args> typename L, typename ... Args, typename ... FirstArgs>
    struct extract_args<L<Args ...>, TInherit, FirstArgs ...> : TInherit<FirstArgs ..., Args ...> {};

    template<typename TArguments, typename TCond, typename ... Args>
    struct argument_validity_checker : public std::false_type {};

    template<typename TArguments, typename TCond>
    struct argument_validity_checker<TArguments, TCond> : public std::true_type {};

    template<typename TArguments, typename TCond, typename T>
    struct argument_validity_checker<TArguments, TCond, T> : public std::true_type {};

    template<typename TArguments, typename T1, typename T2, typename ... Args>
    struct argument_validity_checker<TArguments, std::enable_if_t<
        (detail::get_arg_strictness<T1>() >= detail::get_arg_strictness<T2>()) &&
            (!detail::is_argument_sink_v<config::detail::field_t<T1>> ||
                !detail::is_argument_sink_v<config::detail::field_t<T2>>)
        >, T1, T2, Args ...> : public argument_validity_checker<TArguments, void, T2, Args ...> {};

    template<typename>
    struct argument_fields_validator : public std::false_type {};

    template<template<typename ...> typename NTuple, typename ... NTupleArgs>
    struct argument_fields_validator<NTuple<NTupleArgs ...>> {
        static const bool value = ((!detail::is_any_option_v<config::detail::field_t<NTupleArgs>>) && ...);
    };

    template<typename TArguments>
    constexpr bool validate_arguments() {
        static_assert(std::is_default_constructible_v<TArguments>, "TOptions must be default-constructible");
        static_assert(is_arguments_v<TArguments>, "TOptions must inherit from cppcmd::options");

        static_assert(extract_args<decltype(rfl::to_view(std::declval<TArguments&>())), argument_validity_checker, TArguments, void>::value,
            "All optional arguments must be at the end of the structure");

        static_assert(argument_fields_validator<decltype(rfl::to_view(std::declval<TArguments&>()))>::value,
            "Argument type is not allowed to have 'option' members");

        return true;
    }

}

#endif //CPPCMD_TYPE_VALIDATOR_H