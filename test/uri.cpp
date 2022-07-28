#include <asyncpp/curl/uri.h>
#include <gtest/gtest.h>

using namespace asyncpp::curl;

TEST(ASYNCPP_CURL, UriParse) {
	std::array<std::string, 7> parts{std::string{"http://"},	  std::string{"user:pass@"}, std::string{"example.org"}, std::string{":81"},
									 std::string{"/path/random"}, std::string{"?params"},	 std::string{"#fragment"}};
	for (size_t i = 0; i < (1 << parts.size()); i++) {
		std::string test_url;
		for (size_t x = 0; x < parts.size(); x++) {
			if (i & (1 << x)) test_url += parts[x];
		}
		auto parsed = uri::parse(test_url);
		bool should_be_valid = (i & 0x01);
		if (!should_be_valid) {
			ASSERT_FALSE(parsed.has_value());
			continue;
		}
		ASSERT_TRUE(parsed.has_value());
		auto& u = *parsed;
		if (i & 0x01)
			ASSERT_EQ("http", u.scheme());
		else
			ASSERT_EQ("", u.scheme());
		if (i & 0x02)
			ASSERT_EQ("user:pass", u.auth());
		else
			ASSERT_EQ("", u.auth());
		if (i & 0x04)
			ASSERT_EQ("example.org", u.host());
		else
			ASSERT_EQ("", u.host());
		if (i & 0x08)
			ASSERT_EQ(81, u.port());
		else
			ASSERT_EQ(-1, u.port());
		if (i & 0x10)
			ASSERT_EQ("/path/random", u.path());
		else
			ASSERT_EQ("", u.path());
		if (i & 0x20)
			ASSERT_EQ("params", u.query());
		else
			ASSERT_EQ("", u.query());
		if (i & 0x40)
			ASSERT_EQ("fragment", u.fragment());
		else
			ASSERT_EQ("", u.fragment());
	}
}

TEST(ASYNCPP_CURL, UriParseIPv6) {
	auto parsed = uri::parse("http://user:pass@[fe88:a8ad:686a:3e31:2b25:eb00:27b2:c87b]:81/path/random?params#fragment");
	ASSERT_TRUE(parsed.has_value());
	auto& u = *parsed;
	ASSERT_EQ("http", u.scheme());
	ASSERT_EQ("user:pass", u.auth());
	ASSERT_EQ("[fe88:a8ad:686a:3e31:2b25:eb00:27b2:c87b]", u.host());
	ASSERT_EQ(81, u.port());
	ASSERT_EQ("/path/random", u.path());
	ASSERT_EQ("params", u.query());
	ASSERT_EQ("fragment", u.fragment());
}

TEST(ASYNCPP_CURL, URIToString) {
	uri u{"http://user:pass@example.org:81/path/random?params#fragment"};
	ASSERT_EQ("http://user:pass@example.org:81/path/random?params#fragment", u.to_string());
}

TEST(ASYNCPP_CURL, URIPathSplitMerge) {
	constexpr auto path = "/hello/world/1245/";
	auto parts = uri::split_path(path);
	ASSERT_EQ(3, parts.size());
	ASSERT_EQ("hello", parts[0]);
	ASSERT_EQ("world", parts[1]);
	ASSERT_EQ("1245", parts[2]);
	ASSERT_EQ(path, uri::merge_path(parts.begin(), parts.end()));
}

TEST(ASYNCPP_CURL, URIParseQuery) {
	uri u{"http://test.com?param1&param2=value&param3&param2"};
	auto& parsed = u.query_parsed();
	ASSERT_EQ(4, parsed.size());
	ASSERT_EQ(1, parsed.count("param1"));
	ASSERT_EQ(2, parsed.count("param2"));
	ASSERT_EQ(1, parsed.count("param3"));
	ASSERT_EQ("", parsed.find("param1")->second);
	if (auto it = parsed.find("param2"); it->second == "") {
		ASSERT_EQ("value", (++it)->second);
	} else {
		ASSERT_EQ("value", it->second);
		ASSERT_EQ("", (++it)->second);
	}
	ASSERT_EQ("", parsed.find("param3")->second);
}
