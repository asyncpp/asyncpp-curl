#include <asyncpp/curl/exception.h>
#include <curl/curl.h>

namespace asyncpp::curl {
	const char* exception::what() const noexcept {
		if (m_is_multi)
			return curl_multi_strerror(static_cast<CURLMcode>(m_code));
		else
			return curl_easy_strerror(static_cast<CURLcode>(m_code));
	}
} // namespace asyncpp::curl
