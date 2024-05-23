#ifndef CPPCMD_HELPER_H
#define CPPCMD_HELPER_H

namespace cppcmd {

    template<typename T>
    struct factory {
    private:
        // ideally, only const functions would be allowed, to make default_value, implicit_value
        // and implicit_single_value not use mutable lambda, but it is not possible with current
        // std::function implementation
        std::function<T()> f;

    public:
        static_assert(!std::is_same_v<T, void>, "Factory type cannot be void");

        template<typename F>
        explicit factory(F&& f)
            : f(std::forward<F>(f)) {}

        T value() {
            return f();
        }
    };

    template<typename F>
    factory(F&& f) -> factory<decltype(f())>;

}

#endif //CPPCMD_HELPER_H
