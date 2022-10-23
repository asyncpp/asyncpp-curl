#pragma once
#include <bit>
#include <string>
#include <string_view>
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
		enum class mode { none, normal, strict, pedantic, extreme };
		enum class result { invalid, valid, valid_incomplete };
		/**
		 * \brief Validate a string_view in the specified mode
		 * 
		 * \tparam m The check mode
		 * \param sv The string to validate
		 * \param last_pos if non nullptr this is set to the start of the first erroneous codepoint or sv.end() if the string is valid
		 * \return Result of the check
		 */
		template<mode m = mode::normal>
		constexpr result validate(std::string_view sv, std::string_view::const_iterator* last_pos = nullptr) const noexcept {
			if constexpr (m == mode::none) {
				if (last_pos) *last_pos = sv.end();
				return result::valid;
			}
			for (auto it = sv.begin(); it != sv.end(); it++) {
				if (last_pos) *last_pos = it;
				unsigned char c = static_cast<unsigned char>(*it);
				auto bits = std::countl_one(c);
				// Note: We parse the codepoint on every mode, but the compiler is
				//       smart enough to optimize it away even at -O1 if its never read.
				uint32_t codepoint = (c & ((1 << (7 - bits)) - 1));
				switch (bits) {
				case 0: continue;
				case 1: return result::invalid; // 10xxxxxx is continuation but not at the start
				case 6:
					if constexpr (m != mode::normal) return result::invalid;
					if (++it == sv.end()) {
						if constexpr (m == mode::normal || m == mode::strict)
							return result::valid_incomplete;
						else
							return result::invalid;
					}
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return result::invalid;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 5:
					if constexpr (m != mode::normal) return result::invalid;
					if (++it == sv.end()) {
						if constexpr (m == mode::normal || m == mode::strict)
							return result::valid_incomplete;
						else
							return result::invalid;
					}
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return result::invalid;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 4:
					if (++it == sv.end()) {
						if constexpr (m == mode::normal || m == mode::strict)
							return result::valid_incomplete;
						else
							return ((codepoint << 18) > 0x10FFFF) ? result::invalid : result::valid_incomplete;
					}
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return result::invalid;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 3:
					if (++it == sv.end()) {
						if constexpr (m == mode::normal || m == mode::strict)
							return result::valid_incomplete;
						else
							return ((codepoint << 12) > 0x10FFFF) ? result::invalid : result::valid_incomplete;
					}
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return result::invalid;
					codepoint = (codepoint << 6) | (c & 0x3f);
					[[fallthrough]];
				case 2:
					if (++it == sv.end()) {
						if constexpr (m == mode::normal || m == mode::strict)
							return result::valid_incomplete;
						else
							return ((codepoint << 6) > 0x10FFFF) ? result::invalid : result::valid_incomplete;
					}
					c = static_cast<unsigned char>(*it);
					if ((c & 0xc0) != 0x80) return result::invalid;
					codepoint = (codepoint << 6) | (c & 0x3f);
					break;
				default: return result::invalid;
				}
				if constexpr (m == mode::strict || m == mode::pedantic || m == mode::extreme) {
					//std::cout << codepoint << " " << bits << std::endl;
					// Validate that the smallest possible encoding was used
					if (codepoint < 128 && bits != 0) return result::invalid;
					if (codepoint < 2048 && bits > 2) return result::invalid;
					if (codepoint < 65536 && bits > 3) return result::invalid;
					if (codepoint < 1048576 && bits > 4) return result::invalid;
				}
				if constexpr (m == mode::pedantic || m == mode::extreme) {
					// Surrogate values are disallowed
					if (codepoint >= 0xd800 && codepoint <= 0xdfff) return result::invalid;
					// 0xfeff should never appear in a valid utf-8 sequence
					if ((codepoint & 0xffe0ffffu) == 0xfeff) return result::invalid;
					// 0xfdd0 - 0xfdef are allocated for "application specific purposes"
					if (codepoint >= 0xfdd0 && codepoint <= 0xfdef) return result::invalid;
					// Values above 0x10FFFF are disallowed since 2000 because of UTF-16
					// See http://www.unicode.org/L2/L2000/00079-n2175.htm
					if (codepoint > 0x10FFFF) return result::invalid;
				}
				if constexpr (m == mode::extreme) {
					// 0xffff and 0xfeff should never appear in a valid utf-8 sequence
					if ((codepoint & 0xffe0ffffu) == 0xffff || (codepoint & 0xffe0ffffu) == 0xfffe) return result::invalid;
				}
			}
			if (last_pos) *last_pos = sv.end();
			return result::valid;
		}
		/**
		 * \brief Validate a string_view in the specified mode
		 * 
		 * \param sv The string to validate
		 * \param m The check mode
		 * \param last_pos if non nullptr this is set to the start of the first erroneous codepoint or sv.end() if the string is valid
		 * \return Result of the check
		 */
		constexpr result operator()(std::string_view sv, mode m = mode::normal, std::string_view::const_iterator* last_pos = nullptr) const noexcept {
			switch (m) {
			case mode::none: return validate<mode::none>(sv, last_pos);
			case mode::normal: return validate<mode::normal>(sv, last_pos);
			case mode::strict: return validate<mode::strict>(sv, last_pos);
			case mode::pedantic: return validate<mode::pedantic>(sv, last_pos);
			case mode::extreme: return validate<mode::extreme>(sv, last_pos);
			default: return result::invalid;
			}
		}
	};

} // namespace asyncpp::curl
