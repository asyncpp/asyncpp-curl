#include <array>
#include <asyncpp/curl/version.h>
#include <curl/curl.h>

#define info static_cast<curl_version_info_data*>(m_info)

namespace asyncpp::curl {
	//static_assert(CURL_VERSION_GSASL == (1 << static_cast<int>(version::feature::gsasl)));

	version::version() noexcept { m_info = curl_version_info(CURLVERSION_NOW); }

	std::string_view version::curl_version() const noexcept { return info->version ? info->version : ""; }

	unsigned int version::curl_version_num() const noexcept { return info->version_num; }

	std::string_view version::host() const noexcept { return info->host ? info->host : ""; }

	bool version::has_feature(feature f) const noexcept { return (info->features & (1 << static_cast<size_t>(f))) != 0; }

	std::string_view version::ssl_version() const noexcept { return info->ssl_version ? info->ssl_version : ""; }

	std::string_view version::libz_version() const noexcept { return info->libz_version ? info->libz_version : ""; }

	cstring_array_iterator::helper version::protocols() const noexcept { return cstring_array_iterator::helper{info->protocols}; }

	std::string_view version::protocol(size_t index) const noexcept {
		auto pos = info->protocols;
		while (index) {
			if (pos == nullptr || *pos == nullptr) break;
			pos++;
			index--;
		}
		return (pos != nullptr && *pos != nullptr && index == 0) ? *pos : "";
	}

	size_t version::protocol_count() const noexcept {
		auto pos = info->protocols;
		while (pos && *pos)
			pos++;
		return pos - info->protocols;
	}

	std::string_view version::ares_version() const noexcept {
		if (info->age >= CURLVERSION_SECOND) return info->ares ? info->ares : "";
		return "";
	}

	int version::ares_version_num() const noexcept {
		if (info->age >= CURLVERSION_SECOND) return info->ares_num;
		return -1;
	}

	std::string_view version::libidn_version() const noexcept {
		if (info->age >= CURLVERSION_THIRD) return info->libidn ? info->libidn : "";
		return "";
	}

#if CURL_AT_LEAST_VERSION(7, 16, 1)
	std::string_view version::iconv_version() const noexcept {
		if (info->age >= CURLVERSION_FOURTH) {
			static std::array<char, 12> str = [](int v) {
				std::array<char, 12> res{};
				if (v == 0) return res;
				snprintf(res.data(), res.size() - 1, "%d.%u", v >> 8, v & 0xff);
				return res;
			}(iconv_num());
			return std::string_view(str.data());
		}
		return "";
	}

	int version::iconv_num() const noexcept {
		if (info->age >= CURLVERSION_FOURTH) return info->iconv_ver_num;
		return -1;
	}

	std::string_view version::libssh_version() const noexcept {
		if (info->age >= CURLVERSION_FOURTH) return info->libssh_version ? info->libssh_version : "";
		return "";
	}

#else
	std::string_view version::iconv_version() const noexcept { return ""; }
	int version::iconv_num() const noexcept { return -1; }
	std::string_view version::libssh_version() const noexcept { return ""; }
#endif

#if CURL_AT_LEAST_VERSION(7, 57, 0)
	unsigned int version::brotli_version_num() const noexcept {
		if (info->age >= CURLVERSION_FIFTH) return info->brotli_ver_num;
		return -1;
	}

	std::string_view version::brotli_version() const noexcept {
		if (info->age >= CURLVERSION_FIFTH) return info->brotli_version ? info->brotli_version : "";
		return "";
	}
#else
	unsigned int version::brotli_version_num() const noexcept { return -1; }
	std::string_view version::brotli_version() const noexcept { return ""; }
#endif

#if CURL_AT_LEAST_VERSION(7, 66, 0)
	unsigned int version::nghttp2_version_num() const noexcept {
		if (info->age >= CURLVERSION_SIXTH) return info->nghttp2_ver_num;
		return -1;
	}

	std::string_view version::nghttp2_version() const noexcept {
		if (info->age >= CURLVERSION_SIXTH) return info->nghttp2_version ? info->nghttp2_version : "";
		return "";
	}

	std::string_view version::quic_version() const noexcept {
		if (info->age >= CURLVERSION_SIXTH) return info->quic_version ? info->quic_version : "";
		return "";
	}
#else
	unsigned int version::nghttp2_version_num() const noexcept { return -1; }
	std::string_view version::nghttp2_version() const noexcept { return ""; }
	std::string_view version::quic_version() const noexcept { return ""; }
#endif

#if CURL_AT_LEAST_VERSION(7, 70, 0)
	std::string_view version::cainfo() const noexcept {
		if (info->age >= CURLVERSION_SEVENTH) return info->cainfo ? info->cainfo : "";
		return "";
	}

	std::string_view version::capath() const noexcept {
		if (info->age >= CURLVERSION_SEVENTH) return info->capath ? info->capath : "";
		return "";
	}
#else
	std::string_view version::cainfo() const noexcept { return ""; }
	std::string_view version::capath() const noexcept { return ""; }
#endif

#if CURL_AT_LEAST_VERSION(7, 71, 0)
	unsigned int version::zstd_version_num() const noexcept {
		if (info->age >= CURLVERSION_EIGHTH) return info->zstd_ver_num;
		return -1;
	}

	std::string_view version::zstd_version() const noexcept {
		if (info->age >= CURLVERSION_EIGHTH) return info->zstd_version ? info->zstd_version : "";
		return "";
	}
#else
	unsigned int version::zstd_version_num() const noexcept { return -1; }
	std::string_view version::zstd_version() const noexcept { return ""; }
#endif

#if CURL_AT_LEAST_VERSION(7, 75, 0)
	std::string_view version::hyper_version() const noexcept {
		if (info->age >= CURLVERSION_NINTH) return info->hyper_version ? info->hyper_version : "";
		return "";
	}
#else
	std::string_view version::hyper_version() const noexcept { return ""; }
#endif

#if CURL_AT_LEAST_VERSION(7, 77, 0)
	std::string_view version::gsasl_version() const noexcept {
		if (info->age >= CURLVERSION_TENTH) return info->gsasl_version ? info->gsasl_version : "";
		return "";
	}
#else
	std::string_view version::gsasl_version() const noexcept { return ""; }
#endif

	std::span<const version::feature> version::features() noexcept {
		static constexpr feature list[]{feature::ipv6,		   feature::kerberos4,	 feature::ssl,		 feature::libz,		 feature::ntlm,
										feature::gssnegotiate, feature::debug,		 feature::asynchdns, feature::spnego,	 feature::largefile,
										feature::idn,		   feature::sspi,		 feature::conv,		 feature::curldebug, feature::tlsauth_srp,
										feature::ntlm_wb,	   feature::http2,		 feature::gssapi,	 feature::kerberos5, feature::unix_sockets,
										feature::psl,		   feature::https_proxy, feature::multi_ssl, feature::brotli,	 feature::altsvc,
										feature::http3,		   feature::zstd,		 feature::unicode,	 feature::hsts,		 feature::gsasl};
		return std::span<const feature>{list};
	}

	std::string_view version::feature_name(feature f) noexcept {
		switch (f) {
		case feature::ipv6: return "ipv6";
		case feature::kerberos4: return "kerberos4";
		case feature::ssl: return "ssl";
		case feature::libz: return "libz";
		case feature::ntlm: return "ntlm";
		case feature::gssnegotiate: return "gssnegotiate";
		case feature::debug: return "debug";
		case feature::asynchdns: return "asynchdns";
		case feature::spnego: return "spnego";
		case feature::largefile: return "largefile";
		case feature::idn: return "idn";
		case feature::sspi: return "sspi";
		case feature::conv: return "conv";
		case feature::curldebug: return "curldebug";
		case feature::tlsauth_srp: return "tlsauth_srp";
		case feature::ntlm_wb: return "ntlm_wb";
		case feature::http2: return "http2";
		case feature::gssapi: return "gssapi";
		case feature::kerberos5: return "kerberos5";
		case feature::unix_sockets: return "unix_sockets";
		case feature::psl: return "psl";
		case feature::https_proxy: return "https_proxy";
		case feature::multi_ssl: return "multi_ssl";
		case feature::brotli: return "brotli";
		case feature::altsvc: return "altsvc";
		case feature::http3: return "http3";
		case feature::zstd: return "zstd";
		case feature::unicode: return "unicode";
		case feature::hsts: return "hsts";
		case feature::gsasl: return "gsasl";
		default: return "";
		};
	}

	version::feature version::next_feature(feature f) noexcept {
		switch (f) {
		case feature::ipv6: return feature::kerberos4;
		case feature::kerberos4: return feature::ssl;
		case feature::ssl: return feature::libz;
		case feature::libz: return feature::ntlm;
		case feature::ntlm: return feature::gssnegotiate;
		case feature::gssnegotiate: return feature::debug;
		case feature::debug: return feature::asynchdns;
		case feature::asynchdns: return feature::spnego;
		case feature::spnego: return feature::largefile;
		case feature::largefile: return feature::idn;
		case feature::idn: return feature::sspi;
		case feature::sspi: return feature::conv;
		case feature::conv: return feature::curldebug;
		case feature::curldebug: return feature::tlsauth_srp;
		case feature::tlsauth_srp: return feature::ntlm_wb;
		case feature::ntlm_wb: return feature::http2;
		case feature::http2: return feature::gssapi;
		case feature::gssapi: return feature::kerberos5;
		case feature::kerberos5: return feature::unix_sockets;
		case feature::unix_sockets: return feature::psl;
		case feature::psl: return feature::https_proxy;
		case feature::https_proxy: return feature::multi_ssl;
		case feature::multi_ssl: return feature::brotli;
		case feature::brotli: return feature::altsvc;
		case feature::altsvc: return feature::http3;
		case feature::http3: return feature::zstd;
		case feature::zstd: return feature::unicode;
		case feature::unicode: return feature::hsts;
		case feature::hsts: return feature::gsasl;
		case feature::gsasl:
		default: return f;
		}
	}

} // namespace asyncpp::curl
