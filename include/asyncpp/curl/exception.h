#pragma once
#include <curl/curl.h>
#include <stdexcept>

namespace asyncpp::curl {
	class exception : public std::exception {
		int m_code = -1;
		bool m_is_multi = false;

	public:
		exception(int code, bool is_multi = false) : m_code{code}, m_is_multi{is_multi} {}
		const char* what() const noexcept override {
			if (m_is_multi)
				return curl_multi_strerror(static_cast<CURLMcode>(m_code));
			else
				return curl_easy_strerror(static_cast<CURLcode>(m_code));
		}
		constexpr int code() const noexcept { return m_code; }
	};
} // namespace asyncpp::curl