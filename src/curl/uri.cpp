#include <asyncpp/curl/uri.h>
#include <curl/curl.h>
#include <stdexcept>

namespace asyncpp::curl {
	namespace {
		bool parse_into(std::string_view str, uri& res) {
			// Parse scheme
			auto pos = str.find(":/");
			if (pos == std::string::npos) return false;
			res.scheme(str.substr(0, pos));
			str.remove_prefix(pos + 2);
			if (str.empty()) return false;
			// Double shlash means we can parse a host segment
			if (str[0] == '/') {
				str.remove_prefix(1);
				auto end = str.find_first_of("/?#");
				if (end == std::string::npos) end = str.size();
				auto host = str.substr(0, end);
				str.remove_prefix(end);
				// check if the host has a auth part
				pos = host.find('@');
				if (pos != std::string::npos) {
					res.auth(host.substr(0, pos));
					host.remove_prefix(pos + 1);
				}
				if (!host.empty() && host[0] == '[') {
					// IPv6 address
					pos = host.find(']');
					if (pos == std::string::npos) return false;
					res.host(host.substr(0, pos + 1));
					host.remove_prefix(pos + 1);
					if (!host.empty() && host[0] == ':') {
						host.remove_prefix(1);
						res.port(std::stoi(std::string{host}));
					}
				} else {
					pos = host.rfind(':');
					if (pos != std::string::npos) {
						res.port(std::stoi(std::string{host.substr(pos + 1)}));
						host.remove_suffix(host.size() - pos);
					}
					res.host(host);
				}
			}
			pos = str.find_first_of("?#");
			res.path(str.substr(0, pos));
			if (pos == std::string::npos) return true;
			str.remove_prefix(pos);
			if (str[0] == '?') {
				pos = str.find_first_of("#");
				res.query(str.substr(1, pos - 1));
				if (pos == std::string::npos) return true;
				str.remove_prefix(pos);
			}
			str.remove_prefix(1);
			res.fragment(str);
			return true;
		}
	} // namespace
	uri::uri(std::string_view str) {
		if (!parse_into(str, *this)) throw std::invalid_argument("invalid uri supplied");
	}

	std::optional<uri> uri::parse(std::string_view str) {
		uri res{};
		if (!parse_into(str, res)) return std::nullopt;
		return res;
	}

	std::string uri::to_string() const {
		std::string res;
		if (!m_scheme.empty()) res += m_scheme + "://";
		if (!m_auth.empty()) res += m_auth + "@";
		if (!m_host.empty()) res += m_host;
		if (m_port != -1) res += ":" + std::to_string(m_port);
		if (!m_path.empty()) res += m_path;
		if (!m_query.empty()) res += "?" + m_query;
		if (!m_fragment.empty()) res += "#" + m_fragment;
		return res;
	}

	std::string uri::encode(const std::string_view in) {
		auto ptr = curl_easy_escape(nullptr, in.data(), in.size());
		if (!ptr) throw std::bad_alloc{};
		std::string res{ptr};
		curl_free(ptr);
		return res;
	}

	std::string uri::decode(const std::string_view in) {
		int size = -1;
		auto ptr = curl_easy_unescape(nullptr, in.data(), in.size(), &size);
		if (!ptr || size < 0) throw std::bad_alloc{};
		std::string res{ptr, static_cast<size_t>(size)};
		curl_free(ptr);
		return res;
	}

	std::unordered_multimap<std::string, std::string> uri::parse_formdata(std::string_view data) {
		std::unordered_multimap<std::string, std::string> res;
		size_t offset = 0;
		do {
			auto pos = data.find('&', offset);
			auto part = data.substr(offset, pos == std::string::npos ? std::string::npos : pos - offset);
			if (!part.empty()) {
				auto pos_eq = part.find('=');
				if (pos_eq != std::string::npos)
					res.insert({decode(part.substr(0, pos_eq)), decode(part.substr(pos_eq + 1))});
				else
					res.insert({decode(part), ""});
			}
			if (pos == std::string::npos) break;
			offset = pos + 1;
		} while (offset < data.size());
		return res;
	}

} // namespace asyncpp::curl
