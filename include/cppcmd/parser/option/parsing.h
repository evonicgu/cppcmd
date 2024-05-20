#ifndef CPPCMD_PARSING_H
#define CPPCMD_PARSING_H

#include <string>
#include <vector>
#include <optional>
#include <utility>
#include <unordered_map>

namespace cppcmd::parser {

    struct parsing_result {
        using invocations_t = std::vector<std::pair<std::optional<std::string>, std::size_t>>;

        using long_options_t = std::unordered_map<std::string, invocations_t>;
        using short_options_t = std::unordered_map<char, invocations_t>;

        long_options_t long_options;
        short_options_t short_options;
        std::vector<std::string> arguments;
        int args_used;
    };

    enum class argument_limit {
        single,
        unlimited
    };

}

#endif //CPPCMD_PARSING_H
