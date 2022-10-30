#include <asyncpp/curl/version.h>
#include <gtest/gtest.h>

using namespace asyncpp::curl;

TEST(ASYNCPP_CURL, Version) {
	version info;
	ASSERT_FALSE(info.curl_version().empty());
	ASSERT_FALSE(info.host().empty());
	std::cout << "version: " << info.curl_version();
	std::cout << "\nhost: " << info.host();
	std::cout << "\ncapath: " << info.capath();
	std::cout << "\ncainfo: " << info.cainfo();
	std::cout << "\nfeatures:";
	for (auto f : version::features()) {
		if (info.has_feature(f)) std::cout << " " << version::feature_name(f);
		ASSERT_FALSE(version::feature_name(f).empty());
	}
	std::cout << "\nprotocols:";
	size_t idx = 0;
	for (auto p : info.protocols()) {
		std::cout << " " << p;
		ASSERT_FALSE(p.empty());
		ASSERT_EQ(p, info.protocol(idx));
		idx++;
	}
	ASSERT_EQ(idx, info.protocol_count());
	std::cout << "\nlibz: " << info.libz_version();
	std::cout << "\nares: " << info.ares_version();
	std::cout << "\niconv: " << info.iconv_version();
	std::cout << "\nidn: " << info.libidn_version();
	std::cout << "\nlibssh: " << info.libssh_version();
	std::cout << "\nbrotli: " << info.brotli_version();
	std::cout << "\nnghttp2: " << info.nghttp2_version();
	std::cout << "\nquic: " << info.quic_version();
	std::cout << "\nzstd: " << info.zstd_version();
	std::cout << "\nhyper: " << info.hyper_version();
	std::cout << "\ngsasl: " << info.gsasl_version();
	std::cout << std::endl;
}
