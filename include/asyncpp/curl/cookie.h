#pragma once
#include <algorithm>
#include <asyncpp/curl/uri.h>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <string>

namespace asyncpp::curl {
	class cookie {
	public:
		cookie() = default;
		cookie(std::string_view str) {
			static constexpr auto trim = [](std::string_view str) {
				auto pos = str.find_first_not_of(" \t\n\v\f\r");
				if (pos == std::string::npos) pos = str.size();
				str.remove_prefix(pos);
				pos = str.find_last_not_of(" \t\n\v\f\r");
				if (pos == std::string::npos) return std::string_view{};
				return str.substr(0, pos + 1);
			};
			static constexpr auto pull_part = [](std::string_view& str) -> std::string_view {
				auto pos = str.find("\t");
				if (pos == std::string::npos) throw std::invalid_argument("invalid cookie string");
				auto res = str.substr(0, pos);
				str.remove_prefix(pos + 1);
				return res;
			};
			static constexpr auto iequals = [](std::string_view a, std::string_view b) {
				return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) { return tolower(a) == tolower(b); });
			};
			m_domain = trim(pull_part(str));
			m_include_subdomains = iequals(trim(pull_part(str)), "TRUE");
			m_path = trim(pull_part(str));
			m_secure = iequals(trim(pull_part(str)), "TRUE");
			m_expires = std::chrono::system_clock::from_time_t(std::stoull(std::string{trim(pull_part(str))}));
			m_name = trim(pull_part(str));
			m_value = trim(str);
		}
		cookie(const char* str) : cookie(std::string_view{str}) {}
		cookie(const std::string& str) : cookie(std::string_view{str}) {}
		cookie(std::string name, std::string value) : m_name{std::move(name)}, m_value{value} {}
		cookie(std::string domain, bool subdomain, std::string path, bool secure, std::chrono::system_clock::time_point expires, std::string name,
			   std::string value)
			: m_domain{std::move(domain)}, m_include_subdomains{subdomain}, m_path{std::move(path)}, m_secure{secure}, m_expires{expires},
			  m_name{std::move(name)}, m_value{std::move(value)} {}
		cookie(const cookie&) = default;
		cookie(cookie&&) = default;
		cookie& operator=(const cookie&) = default;
		cookie& operator=(cookie&&) = default;

		const std::string& domain() const noexcept { return m_domain; };
		const std::string& path() const noexcept { return m_path; };
		std::chrono::system_clock::time_point expires() const noexcept { return m_expires; };
		const std::string& name() const noexcept { return m_name; };
		const std::string& value() const noexcept { return m_value; };
		bool include_subdomains() const noexcept { return m_include_subdomains; };
		bool secure() const noexcept { return m_secure; };

		void domain(std::string val) { m_domain = std::move(val); }
		void path(std::string val) { m_path = std::move(val); }
		void expires(std::chrono::system_clock::time_point val) { m_expires = val; }
		void name(std::string val) { m_name = std::move(val); }
		void value(std::string val) { m_value = std::move(val); }
		void include_subdomains(bool val) { m_include_subdomains = val; }
		void secure(bool val) { m_secure = val; }

		bool is_valid() const noexcept { return !m_domain.empty() && !m_path.empty() && !m_name.empty(); }
		bool is_expired() const noexcept { return m_expires <= std::chrono::system_clock::now(); }

		std::string to_string() const {
			std::stringstream res;
			res << m_domain << "\t";
			res << (m_include_subdomains ? "TRUE" : "FALSE") << "\t";
			res << m_path << "\t";
			res << (m_secure ? "TRUE" : "FALSE") << "\t";
			res << std::chrono::system_clock::to_time_t(m_expires) << "\t";
			res << m_name << "\t";
			res << m_value;
			return res.str();
		}

		friend bool operator==(const cookie& lhs, const cookie& rhs) noexcept {
			return std::tie(lhs.m_domain, lhs.m_include_subdomains, lhs.m_path, lhs.m_secure, lhs.m_expires, lhs.m_name, lhs.m_value) ==
				   std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value);
		}
		friend bool operator!=(const cookie& lhs, const cookie& rhs) noexcept { return !(lhs == rhs); }
		friend bool operator<(const cookie& lhs, const cookie& rhs) noexcept {
			return std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value) <
				   std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value);
		}
		friend bool operator>(const cookie& lhs, const cookie& rhs) noexcept {
			return std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value) >
				   std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value);
		}
		friend bool operator<=(const cookie& lhs, const cookie& rhs) noexcept {
			return std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value) <=
				   std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value);
		}
		friend bool operator>=(const cookie& lhs, const cookie& rhs) noexcept {
			return std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value) >=
				   std::tie(rhs.m_domain, rhs.m_include_subdomains, rhs.m_path, rhs.m_secure, rhs.m_expires, rhs.m_name, rhs.m_value);
		}

	private:
		std::string m_domain;
		std::string m_path;
		std::chrono::system_clock::time_point m_expires;
		std::string m_name;
		std::string m_value;
		bool m_include_subdomains;
		bool m_secure;
	};
} // namespace asyncpp::curl