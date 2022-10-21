#pragma once
#include <bit>
#include <string>
#include <vector>

namespace asyncpp::curl {
	struct case_insensitive_less {
		bool operator()(const std::string& lhs, const std::string& rhs) const {
			return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
												[](const char c1, const char c2) { return tolower(c1) < tolower(c2); });
		}
	};

	template<typename T>
	inline void string_ltrim(T& s) {
		auto last = s.find_first_not_of(" \t\f\v\n\r");
		s.erase(0, last);
	}

	template<typename T>
	inline void string_rtrim(T& s) {
		auto last = s.find_last_not_of(" \t\f\v\n\r");
		if (last != T::npos) s.erase(last + 1);
	}

	template<typename T>
	inline void string_trim(T& s) {
		string_rtrim(s);
		string_ltrim(s);
	}

	template<typename TStr, typename TDelim>
	inline std::vector<TStr> string_split(const TStr& str, const TDelim& delim) {
		std::vector<TStr> res;
		size_t offset = 0;
		do {
			size_t pos = str.find(delim, offset);
			res.emplace_back(str.substr(offset, (pos == TStr::npos) ? pos : (pos - offset)));
			offset = (pos != TStr::npos) ? pos + delim.length() : pos;
		} while (offset != TStr::npos);
		return res;
	}

	struct utf8_validator {
		enum class mode { none, normal, strict, pedantic };
		template<mode m = mode::normal>
		constexpr bool validate(std::string_view sv) const noexcept {
			if constexpr (m == mode::none) return true;
			for (auto it = sv.begin(); it != sv.end(); it++) {
				unsigned char c = static_cast<unsigned char>(*it);
				auto bits = std::countl_one(c);
				//std::cout << bits << " " << std::hex << static_cast<int>(c) << std::dec << std::endl;
				// Note: We parse the codepoint on every mode, but the compiler is
				//       smart enough to optimize it away even at -O1 if its never read.
				uint32_t codepoint = (c & ((1 << (7 - bits)) - 1));
				switch (bits) {
				case 0: continue;
				case 1: return false; // 10xxxxxx is continuation but not at the start
				case 6:
					if constexpr (m != mode::normal) return false;
					if (++it == sv.end()) return false;
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return false;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 5:
					if constexpr (m != mode::normal) return false;
					if (++it == sv.end()) return false;
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return false;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 4:
					if (++it == sv.end()) return false;
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return false;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 3:
					if (++it == sv.end()) return false;
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return false;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 2:
					if (++it == sv.end()) return false;
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return false;
					codepoint = (codepoint << 6) | (c & 0x3f);
					break;
				default: return false;
				}
				if constexpr (m != mode::normal) {
					//std::cout << codepoint << " " << bits << std::endl;
					// Validate that the smallest possible encoding was used
					if (codepoint < 128 && bits != 0) return false;
					if (codepoint < 2048 && bits > 2) return false;
					if (codepoint < 65536 && bits > 3) return false;
					if (codepoint < 1048576 && bits > 4) return false;
					// Surrogate values are disallowed
					if (codepoint >= 0xd800 && codepoint <= 0xdfff) return false;
				}
				if constexpr (m == mode::pedantic) {
					// 0xffff and 0xfffe should never appear in a valid utf-8 sequence
					if ((codepoint & 0xffe0ffffu) == 0xffff || (codepoint & 0xffe0ffffu) == 0xfffe) return false;
					// 0xfdd0 - 0xfdef are allocated for "application specific purposes"
					if (codepoint >= 0xfdd0 && codepoint <= 0xfdef) return false;
				}
			}
			return true;
		}
		constexpr bool operator()(std::string_view sv, mode m = mode::normal) const noexcept {
			switch (m) {
			case mode::none: return validate<mode::none>(sv);
			case mode::normal: return validate<mode::normal>(sv);
			case mode::strict: return validate<mode::strict>(sv);
			case mode::pedantic: return validate<mode::pedantic>(sv);
			default: return false;
			}
		}
	};

} // namespace asyncpp::curl
