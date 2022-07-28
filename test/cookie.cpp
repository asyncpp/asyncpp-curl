#include <asyncpp/curl/cookie.h>
#include <chrono>
#include <gtest/gtest.h>
#include <string_view>
#include <system_error>

using namespace asyncpp::curl;

TEST(ASYNCPP_CURL, Cookie) {
	constexpr std::string_view line = "example.com\tFALSE\t/foobar/\tTRUE\t1462299217\tperson\tdaniel\n";

	cookie c{line};
	ASSERT_EQ(c.domain(), "example.com");
	ASSERT_EQ(c.include_subdomains(), false);
	ASSERT_EQ(c.path(), "/foobar/");
	ASSERT_EQ(c.secure(), true);
	ASSERT_EQ(c.expires(), std::chrono::system_clock::from_time_t(1462299217));
	ASSERT_EQ(c.name(), "person");
	ASSERT_EQ(c.value(), "daniel");

	ASSERT_EQ(c.to_string() + "\n", line);
}
