#include <asyncpp/curl/base64.h>
#include <gtest/gtest.h>

using namespace asyncpp::curl;

TEST(ASYNCPP_CURL, Base64Encode) {
	ASSERT_EQ(base64::encode("Hel"), "SGVs");
	ASSERT_EQ(base64::encode("Hell"), "SGVsbA==");
	ASSERT_EQ(base64::encode("Hello"), "SGVsbG8=");
}

TEST(ASYNCPP_CURL, Base64UrlEncode) {
	ASSERT_EQ(base64url::encode("Hel"), "SGVs");
	ASSERT_EQ(base64url::encode("Hell"), "SGVsbA");
	ASSERT_EQ(base64url::encode("Hello"), "SGVsbG8");
}

TEST(ASYNCPP_CURL, Base64Decode) {
	ASSERT_EQ(base64::decode("SGVs"), "Hel");
	ASSERT_EQ(base64::decode("SGVsbA=="), "Hell");
	ASSERT_EQ(base64::decode("SGVsbG8="), "Hello");
}

TEST(ASYNCPP_CURL, Base64UrlDecode) {
	ASSERT_EQ(base64url::decode("SGVs"), "Hel");
	ASSERT_EQ(base64url::decode("SGVsbA"), "Hell");
	ASSERT_EQ(base64url::decode("SGVsbG8"), "Hello");
}