#include "asyncpp/curl/cookie.h"
#include <asyncpp/curl/webclient.h>
#include <asyncpp/sync_wait.h>
#include <asyncpp/task.h>
#include <gtest/gtest.h>

using namespace asyncpp::curl;

TEST(ASYNCPP_CURL, WebClientSync) {
	auto req = http_request::make_get("http://www.google.de");
	auto resp = req.execute_sync();
	ASSERT_EQ(resp.status_code, 200);
	ASSERT_EQ(resp.status_message, "OK");
	ASSERT_FALSE(resp.headers.empty());
	ASSERT_FALSE(resp.body.empty());
}

TEST(ASYNCPP_CURL, WebClientAsync) {
	auto req = http_request::make_get("http://www.google.de");
	auto resp = asyncpp::as_promise(req.execute_async()).get();
	ASSERT_EQ(resp.status_code, 200);
	ASSERT_EQ(resp.status_message, "OK");
	ASSERT_FALSE(resp.headers.empty());
	ASSERT_FALSE(resp.body.empty());
}

TEST(ASYNCPP_CURL, WebClientCookies) {
	auto req = http_request::make_get("https://www.google.de/");
	auto resp = req.execute_sync();
	ASSERT_EQ(resp.status_code, 200);
	ASSERT_EQ(resp.status_message, "OK");
	ASSERT_FALSE(resp.headers.empty());
	ASSERT_FALSE(resp.body.empty());
	ASSERT_FALSE(resp.cookies.empty());
}

TEST(ASYNCPP_CURL, WebClientHeadersInsensitive) {
	http_request req;
	req.headers.emplace("Hello", "World");
	req.headers.emplace("hello", "Sun");
	req.headers.emplace("goodBye", "Sun");

	ASSERT_EQ(3, req.headers.size());
	ASSERT_EQ(2, req.headers.count("HELLO"));
	ASSERT_EQ(1, req.headers.count("GOODBYE"));
}
