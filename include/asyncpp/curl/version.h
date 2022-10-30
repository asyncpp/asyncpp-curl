#pragma once
#include <span>
#include <string>
#include <string_view>

namespace asyncpp::curl {
	class cstring_array_iterator {
		const char* const* m_pos{nullptr};

	public:
		struct helper;
		constexpr cstring_array_iterator() noexcept = default;
		constexpr cstring_array_iterator(const char* const* array) noexcept : m_pos(array) {}
		constexpr cstring_array_iterator(const cstring_array_iterator& other) noexcept = default;
		constexpr cstring_array_iterator& operator=(const cstring_array_iterator& other) noexcept = default;

		constexpr cstring_array_iterator& operator++() noexcept {
			if (m_pos && *m_pos) m_pos++;
			return *this;
		}
		constexpr cstring_array_iterator operator++(int) noexcept {
			auto temp = *this;
			++(*this);
			return temp;
		}

		constexpr bool operator==(const cstring_array_iterator& other) const noexcept {
			if (other.m_pos == nullptr) return m_pos == nullptr || *m_pos == nullptr;
			return m_pos == other.m_pos;
		}
		constexpr bool operator!=(const cstring_array_iterator& other) const noexcept { return !(*this == other); }

		constexpr std::string_view operator*() const noexcept {
			if (m_pos == nullptr || *m_pos == nullptr) return {};
			return *m_pos;
		}
	};
	struct cstring_array_iterator::helper {
		cstring_array_iterator iterator;
		constexpr cstring_array_iterator begin() const noexcept { return iterator; }
		constexpr cstring_array_iterator end() const noexcept { return {}; }
	};
	class version {
		void* m_info;

	public:
		enum class feature;
		version() noexcept;
		constexpr version(const version& other) noexcept : m_info(other.m_info) {}
		constexpr version& operator=(const version& other) noexcept {
			m_info = other.m_info;
			return *this;
		}

		std::string_view curl_version() const noexcept;
		unsigned int curl_version_num() const noexcept;
		std::string_view host() const noexcept;
		bool has_feature(feature f) const noexcept;
		std::string_view ssl_version() const noexcept;
		std::string_view libz_version() const noexcept;
		cstring_array_iterator::helper protocols() const noexcept;
		std::string_view protocol(size_t index) const noexcept;
		size_t protocol_count() const noexcept;

		/* The fields below this were added in CURLVERSION_SECOND */
		std::string_view ares_version() const noexcept;
		int ares_version_num() const noexcept;

		/* This field was added in CURLVERSION_THIRD */
		std::string_view libidn_version() const noexcept;

		/* These field were added in CURLVERSION_FOURTH */
		std::string_view iconv_version() const noexcept;
		int iconv_num() const noexcept;
		std::string_view libssh_version() const noexcept;

		/* These fields were added in CURLVERSION_FIFTH */
		unsigned int brotli_version_num() const noexcept;
		std::string_view brotli_version() const noexcept;

		/* These fields were added in CURLVERSION_SIXTH */
		unsigned int nghttp2_version_num() const noexcept;
		std::string_view nghttp2_version() const noexcept;
		std::string_view quic_version() const noexcept;

		/* These fields were added in CURLVERSION_SEVENTH */
		std::string_view cainfo() const noexcept;
		std::string_view capath() const noexcept;

		/* These fields were added in CURLVERSION_EIGHTH */
		unsigned int zstd_version_num() const noexcept;
		std::string_view zstd_version() const noexcept;

		/* These fields were added in CURLVERSION_NINTH */
		std::string_view hyper_version() const noexcept;

		/* These fields were added in CURLVERSION_TENTH */
		std::string_view gsasl_version() const noexcept;

		enum class feature {
			ipv6,
			kerberos4,
			ssl,
			libz,
			ntlm,
			gssnegotiate,
			debug,
			asynchdns,
			spnego,
			largefile,
			idn,
			sspi,
			conv,
			curldebug,
			tlsauth_srp,
			ntlm_wb,
			http2,
			gssapi,
			kerberos5,
			unix_sockets,
			psl,
			https_proxy,
			multi_ssl,
			brotli,
			altsvc,
			http3,
			zstd,
			unicode,
			hsts,
			gsasl
		};
		static std::span<const feature> features() noexcept;
		static std::string_view feature_name(feature f) noexcept;
		static feature next_feature(feature f) noexcept;
	};
} // namespace asyncpp::curl
