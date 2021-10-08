#pragma once
#include <stdexcept>

namespace asyncpp::curl {
    class exception : public std::exception {
        int m_code = -1;
        bool m_is_multi = false;
    public:
        exception(int code, bool is_multi = false) : m_code{code}, m_is_multi{is_multi} {}
        const char* what() const noexcept override;
        constexpr int code() const noexcept { return m_code; }
    };
}