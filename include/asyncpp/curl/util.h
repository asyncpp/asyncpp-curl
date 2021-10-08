#pragma once
#include <bit>
#include <string>

namespace asyncpp::curl::util {
	std::string escape(const std::string& in);
	std::string unescape(const std::string& in);
	bool equals_ignorecase(const std::string& a, const std::string& b);
	void get_random(void* buffer, size_t len);
	bool starts_with_ignorecase(const std::string& a, const std::string& b);
	std::string base64_encode(const std::string_view data);
	std::string base64_decode(const std::string_view data);
	std::string base64url_encode(const std::string_view data);
	std::string base64url_decode(const std::string_view data);

	template<typename T>
	inline T endian_swap(T val) noexcept requires(std::is_standard_layout_v<T>&& std::is_trivial_v<T>) {
		uint8_t* ptr = reinterpret_cast<uint8_t*>(&val);
		std::reverse(ptr, ptr + sizeof(T));
		return val;
	}

	template<typename T>
	inline T native_to_big(T val) noexcept requires(std::is_standard_layout_v<T>&& std::is_trivial_v<T>) {
		if constexpr (std::endian::native != std::endian::big) return endian_swap<T>(val);
		return val;
	}

	template<typename T>
	inline T native_to_little(T val) noexcept requires(std::is_standard_layout_v<T>&& std::is_trivial_v<T>) {
		if constexpr (std::endian::native != std::endian::little) return endian_swap<T>(val);
		return val;
	}

	template<typename T>
	inline void set(void* buffer, T val) noexcept requires(std::is_standard_layout_v<T>&& std::is_trivial_v<T>) {
		memcpy(buffer, &val, sizeof(T));
	}

	template<typename T>
	inline T get(const void* buffer) noexcept requires(std::is_standard_layout_v<T>&& std::is_trivial_v<T>) {
		T res;
		memcpy(&res, buffer, sizeof(T));
		return res;
	}

	// trim from start (in place)
	template<typename StringType>
	inline void ltrim(StringType& s) {
		auto pos = s.find_first_not_of(" \t\n\v\r\f");
		if (pos == StringType::npos) return;
		s = s.substr(pos);
	}

	// trim from end (in place)
	template<typename StringType>
	inline void rtrim(StringType& s) {
		auto pos = s.find_last_not_of(" \t\n\v\r\f");
		if (pos == StringType::npos) return;
		s = s.substr(0, pos + 1);
	}

	// trim from both ends (in place)
	template<typename StringType>
	inline void trim(StringType& s) {
		ltrim<StringType>(s);
		rtrim<StringType>(s);
	}
} // namespace asyncpp::curl::util