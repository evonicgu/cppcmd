#ifndef CPPCMD_VALIDATORS_H
#define CPPCMD_VALIDATORS_H

#include <memory>
#include <vector>
#include <sstream>
#include <filesystem>
#include <optional>
#include <string>

namespace cppcmd {

    using err_t = std::optional<std::string>;

    namespace config {

        template<typename ... TValidators>
        struct validators {
            std::tuple<TValidators ...> given_validators;

            template<typename ... TDeduced>
            explicit validators(TDeduced&& ... given_validators)
                : given_validators(std::forward<TDeduced>(given_validators) ...) {}
        };


        template<typename ... TValidators>
        validators(TValidators&& ...) -> validators<std::remove_cvref_t<TValidators> ...>;

        template<typename T>
        class validator_storage {
            class any_validator {
            public:
                virtual err_t validate(const T&) = 0;

                virtual ~any_validator() = default;
            };

            template<typename TValidator>
            class validator_erased : public any_validator {
                TValidator validator;

            public:
                explicit validator_erased(TValidator validator)
                    : validator(std::move(validator)) {}

                err_t validate(const T& value) override {
                    return validator(value);
                }
            };

            std::vector<std::unique_ptr<any_validator>> stored_validators;

        public:
            template<typename ... TValidators>
            explicit validator_storage(TValidators&& ... given) {
                stored_validators.reserve(sizeof...(given));

                (stored_validators.push_back(std::make_unique<validator_erased<TValidators>>(std::move(given))), ...);
            }

            template<typename ... TValidators>
            validator_storage(validators<TValidators ...> given) {
                stored_validators.reserve(sizeof...(TValidators));

                static_assert((std::is_invocable_v<TValidators, T> && ...),
                    "Each of the validators must be invocable with T");

                init(std::move(given), std::make_index_sequence<sizeof...(TValidators)>());
            }

            err_t validate(const T& value) {
                for (auto& validator : stored_validators) {
                    auto result = validator->validate(value);

                    if (result.has_value()) {
                        return std::move(result);
                    }
                }

                return std::nullopt;
            }

        private:
            template<typename ... TValidators, std::size_t ... Is>
            void init(validators<TValidators ...>&& given, std::index_sequence<Is ...>) {
                (stored_validators.push_back(std::make_unique<validator_erased<TValidators>>(std::move(
                    std::get<Is>(given.given_validators)))), ...);
            }
        };

    }

    namespace validators {

        struct size {
            std::size_t min, max;

            size(std::size_t min, std::size_t max)
                : min(min),
                  max(max) {}

            template<typename T>
            err_t operator()(const T& value) {
                if (value.size() < min || value.size() > max) {
                    return "Value size is required to be between '" +
                            std::to_string(min) + "' and '" + std::to_string(max) + '\'';
                }

                return std::nullopt;
            }
        };

        template<typename T, typename Cond = void>
        struct in {
        private:
            std::vector<T> values;

        public:
            explicit in(std::vector<T> values)
                : values(std::move(values)) {}

            in(std::initializer_list<T> values)
                : values(std::move(values)) {}

            template<typename Tp>
            err_t operator()(const Tp& value) {
                for (auto& v : values) {
                    if (v == value) {
                        return std::nullopt;
                    }
                }

                std::ostringstream ss;

                ss << "Unexpected value '" << value << "', allowed values are: ";

                bool first = true;

                for (auto& v : values) {
                    if (!first) {
                        ss << ", ";
                    }

                    ss << '\'' << v << '\'';
                }

                return ss.str();
            }
        };

        struct existing_file {
            template<typename T>
            err_t operator()(const T& value) {
                const std::filesystem::path p(value);

                if (!std::filesystem::exists(p)) {
                    return "File '" + p.string() + "' does not exist";
                }

                if (std::filesystem::is_directory(p)) {
                    return '\'' + p.string() + "' is not a file";
                }

                return std::nullopt;
            }
        };

        struct permissions {
        private:
            std::filesystem::perms required_permissions;

        public:
            explicit permissions(std::filesystem::perms required_permissions)
                : required_permissions(required_permissions) {}

            template<typename T>
            err_t operator()(const T& value) {
                const std::filesystem::path p(value);

                std::error_code ec;
                auto status = std::filesystem::status(p, ec);

                if (ec) {
                    return "Failed to get permissions for file or directory: '" + p.string() + '\'';
                }

                err_t error;

                check(error, status,
                    std::filesystem::perms::group_read |
                    std::filesystem::perms::others_read |
                    std::filesystem::perms::owner_read,
                    "is not readable", p);

                check(error, status,
                    std::filesystem::perms::group_write |
                    std::filesystem::perms::others_write |
                    std::filesystem::perms::owner_write,
                    "is not writable", p);

                check(error, status,
                    std::filesystem::perms::group_exec |
                    std::filesystem::perms::others_exec |
                    std::filesystem::perms::owner_exec,
                    "is not executable", p);

                check(error, status, required_permissions, "does not have required permissions", p);

                return error;
            }

        private:
            void check(err_t& error, const std::filesystem::file_status& status,
                std::filesystem::perms checked_permissions,
                const char* message, const std::filesystem::path& p) {
                if (error.has_value()) {
                    return;
                }

                std::filesystem::perms needed_permissions = checked_permissions & required_permissions;

                if ((status.permissions() & needed_permissions) != needed_permissions) {
                    error = "File or directory '" + p.string() + "\' " + message;
                }
            }
        };

        struct existing_directory {
            template<typename T>
            err_t operator()(const T& value) {
                const std::filesystem::path p(value);

                if (!std::filesystem::exists(p)) {
                    return "Directory '" + p.string() + "' does not exist";
                }

                if (!std::filesystem::is_directory(p)) {
                    return '\'' + p.string() + "' is not a directory";
                }

                return std::nullopt;
            }
        };

        struct existing_path {
            template<typename T>
            err_t operator()(const T& value) {
                const std::filesystem::path p(value);

                if (!std::filesystem::exists(p)) {
                    return '\'' + p.string() + "' is not a file or a directory";
                }

                return std::nullopt;
            }
        };

        struct nonexistent_path {
            template<typename T>
            err_t operator()(const T& value) {
                const std::filesystem::path p(value);

                if (std::filesystem::exists(p)) {
                    return (std::filesystem::is_directory(p) ? "Directory '" : "File '") +
                            p.string() + "' already exists";
                }

                return std::nullopt;
            }
        };

        template<typename T>
        struct in_range {
        private:
            T min, max;

        public:
            explicit in_range(T min, T max)
                : min(std::move(min)),
                  max(std::move(max)) {}

            template<typename Tp>
            err_t operator()(const Tp& value) {
                if (value < min || max < value) {
                    std::ostringstream ss;

                    ss << "Value is required to be between '" << min << "' and '" << max << '\'';

                    return ss.str();
                }

                return std::nullopt;
            }
        };

        template<typename T>
        struct less_than {
        private:
            T max;

        public:
            explicit less_than(T max)
                : max(std::move(max)) {}

            template<typename Tp>
            err_t operator()(const Tp& value) {
                if (max < value || max == value) {
                    std::ostringstream ss;

                    ss << "Value is required to be less than '" << max << '\'';

                    return ss.str();
                }

                return std::nullopt;
            }
        };

        template<typename T>
        struct less_than_eq {
        private:
            T max;

        public:
            explicit less_than_eq(T max)
                : max(std::move(max)) {}

            template<typename Tp>
            err_t operator()(const Tp& value) {
                if (max < value) {
                    std::ostringstream ss;

                    ss << "Value is required to be less than or equal to '" << max << '\'';

                    return ss.str();
                }

                return std::nullopt;
            }
        };

        template<typename T>
        struct greater_than {
        private:
            T min;

        public:
            explicit greater_than(T min)
                : min(std::move(min)) {}

            template<typename Tp>
            err_t operator()(const Tp& value) {
                if (value < min || value == min) {
                    std::ostringstream ss;

                    ss << "Value is required to be greater than '" << min << '\'';

                    return ss.str();
                }

                return std::nullopt;
            }
        };

        template<typename T>
        struct greater_than_eq {
        private:
            T min;

        public:
            explicit greater_than_eq(T min)
                : min(std::move(min)) {}

            template<typename Tp>
            err_t operator()(const Tp& value) {
                if (value < min) {
                    std::ostringstream ss;

                    ss << "Value is required to be greater than or equal to '" << min << '\'';

                    return ss.str();
                }

                return std::nullopt;
            }
        };

        template<typename T>
        struct equal_to {
        private:
            T expected;

        public:
            explicit equal_to(T expected)
                : expected(std::move(expected)) {}

            template<typename Tp>
            err_t operator()(const Tp& value) {
                if (value == expected) {
                    std::ostringstream ss;

                    ss << "Value is required to be equal to '" << expected << '\'';

                    return ss.str();
                }

                return std::nullopt;
            }
        };

    }

}

#endif //CPPCMD_VALIDATORS_H