#pragma once

#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

inline int g_failed = 0;
inline int g_passed = 0;

inline void check(bool condition, const char* expression, const char* file, int line) {
    if (condition) {
        ++g_passed;
        return;
    }
    ++g_failed;
    std::cerr << file << ":" << line << " CHECK failed: " << expression << "\n";
}

template <typename Fn>
void check_throws_msg(Fn&& fn, std::string_view part, const char* expression, const char* file, int line) {
    try {
        std::forward<Fn>(fn)();
        ++g_failed;
        std::cerr << file << ":" << line << " expected exception: " << expression << "\n";
    } catch (const std::invalid_argument& error) {
        if (std::string_view(error.what()).find(part) == std::string_view::npos) {
            ++g_failed;
            std::cerr << file << ":" << line << " unexpected message \"" << error.what()
                      << "\" for " << expression << "\n";
            return;
        }
        ++g_passed;
    } catch (const std::exception& error) {
        ++g_failed;
        std::cerr << file << ":" << line << " wrong exception type: " << error.what() << "\n";
    }
}

#define CHECK(expression) ::check(static_cast<bool>(expression), #expression, __FILE__, __LINE__)
#define CHECK_THROWS_MSG(part, ...) \
    ::check_throws_msg([&] { __VA_ARGS__; }, part, #__VA_ARGS__, __FILE__, __LINE__)
