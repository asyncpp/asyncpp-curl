#include <asyncpp/curl/util.h>
#include <curl/curl.h>
#include <functional>
#include <openssl/evp.h>
#include <random>
#include <stdexcept>

namespace asyncpp::curl::util {

	std::string escape(const std::string_view in) {
		auto ptr = curl_easy_escape(nullptr, in.data(), in.size());
		if (!ptr) throw std::bad_alloc{};
		std::string res{ptr};
		curl_free(ptr);
		return res;
	}

	std::string unescape(const std::string_view in) {
		int size = -1;
		auto ptr = curl_easy_unescape(nullptr, in.data(), in.size(), &size);
		if (!ptr || size < 0) throw std::bad_alloc{};
		std::string res{ptr, static_cast<size_t>(size)};
		curl_free(ptr);
		return res;
	}

	bool equals_ignorecase(const std::string_view a, const std::string_view b) {
		return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) { return tolower(a) == tolower(b); });
	}

	void get_random(void* buffer, size_t len) {
		static std::default_random_engine rd{};
		static std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t> bytes{rd};

		auto u8ptr = reinterpret_cast<uint8_t*>(buffer);
		std::generate(u8ptr, u8ptr + len, std::ref(bytes));
	}

	bool starts_with_ignorecase(const std::string_view a, const std::string_view b) {
		if (a.size() < b.size()) return false;
		return equals_ignorecase(a.substr(0, b.size()), b);
	}

	std::string base64_encode(const std::string_view data) {
		std::string res;
		const int pl = 4 * ((data.size() + 2) / 3);
		res.resize(pl + 1);
		const int ol = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(res.data()), reinterpret_cast<const unsigned char*>(data.data()), data.size());
		if (pl != ol) throw std::logic_error("base64 size missmatch");
		return res;
	}

	std::string base64_decode(const std::string_view data) {
		std::string res;
		const int pl = (3 * data.size()) / 4;
		res.resize(pl + 1);
		const int ol = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(res.data()), reinterpret_cast<const unsigned char*>(data.data()), data.size());
		if (ol == -1) throw std::runtime_error("failed to decode base64");
		if (ol < static_cast<int>(res.size())) res.resize(ol);
		return res;
	}

	std::string base64url_encode(const std::string_view data) {
		auto res = base64_encode(data);
		for (auto& e : res) {
			if (e == '+')
				e = '-';
			else if (e == '/')
				e = '_';
		}
		auto pos = res.find_last_not_of('=');
		if (pos != std::string::npos) res.resize(pos);
		return res;
	}

	std::string base64url_decode(const std::string_view data) {
		std::string d{data};
		for (auto& e : d) {
			if (e == '-')
				e = '+';
			else if (e == '_')
				e = '/';
		}
		d.resize((d.size() + 3) & ~0x3);
		return base64_decode(d);
	}

} // namespace asyncpp::curl::util