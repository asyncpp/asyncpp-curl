#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace asyncpp::curl {
	class uri {
	public:
		uri() = default;
		uri(std::string_view str);
		uri(const char* str) : uri(std::string_view{str}) {}
		uri(const std::string& str) : uri(std::string_view{str}) {}
		uri(const uri&) = default;
		uri(uri&&) = default;
		uri& operator=(const uri&) = default;
		uri& operator=(uri&&) = default;

		const std::string& scheme() const noexcept { return m_scheme; }
		const std::string& auth() const noexcept { return m_auth; }
		const std::string& host() const noexcept { return m_host; }
		int port() const noexcept { return m_port; }
		const std::string& path() const noexcept { return m_path; }
		const std::string& query() const noexcept { return m_query; }
		const std::unordered_multimap<std::string, std::string>& query_parsed() const {
			if (m_parsed_query.has_value()) return *m_parsed_query;
			m_parsed_query = parse_formdata(m_query);
			return *m_parsed_query;
		}
		const std::string& fragment() const noexcept { return m_fragment; }

		std::string path_query() const { return m_query.empty() ? m_path : (m_path + '?' + m_query); }

		void scheme(std::string str) { m_scheme = std::move(str); }
		void scheme(std::string_view str) { m_scheme = std::string(str); }
		void auth(std::string str) { m_auth = std::move(str); }
		void auth(std::string_view str) { m_auth = std::string(str); }
		void host(std::string str) { m_host = std::move(str); }
		void host(std::string_view str) { m_host = std::string(str); }
		void port(int port) { m_port = (std::max)(-1, (std::min)(port, 65535)); }
		void path(std::string str) { m_path = std::move(str); }
		void path(std::string_view str) { m_path = std::string(str); }
		void query(std::string str) { m_query = std::move(str); }
		void query(std::string_view str) { m_query = std::string(str); }
		void fragment(std::string str) { m_fragment = std::move(str); }
		void fragment(std::string_view str) { m_fragment = std::string(str); }

		bool is_empty() const noexcept {
			return m_scheme.empty() && m_auth.empty() && m_host.empty() && m_port == -1 && m_path.empty() && m_query.empty() && m_fragment.empty();
		}
		bool is_relative() const noexcept { return m_host.empty(); }
		bool is_port_default() const noexcept { return port() == -1; }
		bool is_authority() const noexcept { return m_path.empty() && m_query.empty() && m_fragment.empty(); }
		std::string to_string() const;

		static std::string encode(std::string_view str);
		static std::string decode(std::string_view str);
		static std::unordered_multimap<std::string, std::string> parse_formdata(std::string_view data);

		static std::vector<std::string> split_path(std::string_view path) {
			std::vector<std::string> result;
			size_t offset = 0;
			auto pos = path.find('/', offset);
			while (pos != std::string_view::npos) {
				if (auto len = pos - offset; len != 0) result.emplace_back(path.substr(offset, len));
				offset = pos + 1;
				pos = path.find('/', offset);
			}
			if (auto len = path.size() - offset; len != 0) result.emplace_back(path.substr(offset, len));
			return result;
		}

		template<typename ItBegin, typename ItEnd>
		static std::string merge_path(ItBegin it, ItEnd end) {
			std::string result = "/";
			for (; it != end; it++) {
				result += *it;
				result += "/";
			}
			return result;
		}

		static std::optional<uri> parse(std::string_view str);

		friend bool operator==(const uri& lhs, const uri& rhs) noexcept {
			return std::tie(lhs.m_scheme, lhs.m_auth, lhs.m_host, lhs.m_port, lhs.m_path, lhs.m_query, lhs.m_fragment) ==
				   std::tie(rhs.m_scheme, rhs.m_auth, rhs.m_host, rhs.m_port, rhs.m_path, rhs.m_query, rhs.m_fragment);
		}
		friend bool operator!=(const uri& lhs, const uri& rhs) noexcept { return !(lhs == rhs); }
		friend bool operator<(const uri& lhs, const uri& rhs) noexcept {
			return std::tie(lhs.m_scheme, lhs.m_auth, lhs.m_host, lhs.m_port, lhs.m_path, lhs.m_query, lhs.m_fragment) <
				   std::tie(rhs.m_scheme, rhs.m_auth, rhs.m_host, rhs.m_port, rhs.m_path, rhs.m_query, rhs.m_fragment);
		}
		friend bool operator>(const uri& lhs, const uri& rhs) noexcept {
			return std::tie(lhs.m_scheme, lhs.m_auth, lhs.m_host, lhs.m_port, lhs.m_path, lhs.m_query, lhs.m_fragment) >
				   std::tie(rhs.m_scheme, rhs.m_auth, rhs.m_host, rhs.m_port, rhs.m_path, rhs.m_query, rhs.m_fragment);
		}
		friend bool operator<=(const uri& lhs, const uri& rhs) noexcept {
			return std::tie(lhs.m_scheme, lhs.m_auth, lhs.m_host, lhs.m_port, lhs.m_path, lhs.m_query, lhs.m_fragment) <=
				   std::tie(rhs.m_scheme, rhs.m_auth, rhs.m_host, rhs.m_port, rhs.m_path, rhs.m_query, rhs.m_fragment);
		}
		friend bool operator>=(const uri& lhs, const uri& rhs) noexcept {
			return std::tie(lhs.m_scheme, lhs.m_auth, lhs.m_host, lhs.m_port, lhs.m_path, lhs.m_query, lhs.m_fragment) >=
				   std::tie(rhs.m_scheme, rhs.m_auth, rhs.m_host, rhs.m_port, rhs.m_path, rhs.m_query, rhs.m_fragment);
		}

	private:
		std::string m_scheme;
		std::string m_auth;
		std::string m_host;
		int m_port{-1};
		std::string m_path;
		std::string m_query;
		std::string m_fragment;
		mutable std::optional<std::unordered_multimap<std::string, std::string>> m_parsed_query;
	};
} // namespace asyncpp::curl
