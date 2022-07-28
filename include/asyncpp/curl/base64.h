#pragma once
#include <string>

namespace asyncpp::curl {
	class base64 {
	public:
		static std::string encode(std::string_view data);
		static std::string decode(std::string_view data);
	};

	class base64url {
	public:
		static std::string encode(std::string_view data);
		static std::string decode(std::string_view data);
	};
} // namespace asyncpp::curl