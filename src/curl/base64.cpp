#include <asyncpp/curl/base64.h>
#include <stdexcept>

namespace asyncpp::curl {

	std::string base64::encode(const std::string_view data) {
		static constexpr char base64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

		auto olen = 4 * ((data.size() + 2) / 3);
		if (olen < data.size()) throw std::logic_error("internal error");

		std::string res;
		res.resize(olen);
		auto out = res.data();

		const auto end = data.data() + data.size();
		auto in = data.data();
		while (end - in >= 3) {
			*out++ = base64_table[in[0] >> 2];
			*out++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*out++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
			*out++ = base64_table[in[2] & 0x3f];
			in += 3;
		}

		if (end - in) {
			*out++ = base64_table[in[0] >> 2];
			if (end - in == 1) {
				*out++ = base64_table[(in[0] & 0x03) << 4];
				*out++ = '=';
			} else {
				*out++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
				*out++ = base64_table[(in[1] & 0x0f) << 2];
			}
			*out++ = '=';
		}

		return res;
	}

	std::string base64::decode(const std::string_view data) {
		static constexpr unsigned char B64index[256] = {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
											  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63, 52, 53,
											  54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
											  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63, 0,  26, 27, 28,
											  29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51};

		unsigned char* p = (unsigned char*)data.data();
		int pad = data.size() > 0 && (data.size() % 4 || p[data.size() - 1] == '=');
		const size_t L = ((data.size() + 3) / 4 - pad) * 4;
		std::string str;
		str.reserve(((L / 4) + 1) * 3 + pad);
		str.resize(L / 4 * 3 + pad, '\0');

		for (size_t i = 0, j = 0; i < L; i += 4) {
			int n = static_cast<int>(B64index[p[i]]) << 18 | static_cast<int>(B64index[p[i + 1]]) << 12
					| static_cast<int>(B64index[p[i + 2]]) << 6 | static_cast<int>(B64index[p[i + 3]]);
			str[j++] = n >> 16;
			str[j++] = n >> 8 & 0xFF;
			str[j++] = n & 0xFF;
		}
		if (pad) {
			int n = static_cast<int>(B64index[p[L]]) << 18 | static_cast<int>(B64index[p[L + 1]]) << 12;
			str[str.size() - 1] = n >> 16;

			if (data.size() > L + 2 && p[L + 2] != '=') {
				n |= static_cast<int>(B64index[p[L + 2]]) << 6;
				str.push_back(n >> 8 & 0xFF);
			}
		}
		return str;
	}

	std::string base64url::encode(const std::string_view data) {
		auto res = base64::encode(data);
		for (auto& e : res) {
			if (e == '+')
				e = '-';
			else if (e == '/')
				e = '_';
		}
		auto pos = res.find('=');
		if (pos != std::string::npos) res.resize(pos);
		return res;
	}

	std::string base64url::decode(const std::string_view data) {
		std::string d{data};
		for (auto& e : d) {
			if (e == '-')
				e = '+';
			else if (e == '_')
				e = '/';
		}
		d.resize((d.size() + 3) & ~0x3, '=');
		return base64::decode(d);
	}

} // namespace asyncpp::curl
