#include <asyncpp/curl/cookie.h>
#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/webclient.h>
#include <asyncpp/sync_wait.h>
#include <asyncpp/task.h>
#include <curl/curl.h>
#include <gtest/gtest.h>

using namespace asyncpp::curl;

TEST(ASYNCPP_CURL, WebClientSync) {
	auto req = http_request::make_get("https://www.google.de");
	auto resp = req.execute_sync();
	ASSERT_EQ(resp.status_code, 200);
	ASSERT_FALSE(resp.headers.empty());
	ASSERT_FALSE(resp.body.empty());
}

TEST(ASYNCPP_CURL, WebClientAsync) {
	auto req = http_request::make_get("https://www.google.de");
	auto resp = asyncpp::as_promise(req.execute_async()).get();
	ASSERT_EQ(resp.status_code, 200);
	ASSERT_FALSE(resp.headers.empty());
	ASSERT_FALSE(resp.body.empty());
}

TEST(ASYNCPP_CURL, WebClientAsyncStopToken) {
	std::stop_source source;
	auto req = http_request::make_get("https://www.google.de");
	auto resp_future = asyncpp::as_promise(req.execute_async(source.get_token()));
	source.request_stop();
	try {
		resp_future.get();
		FAIL() << "Did not throw";
	} catch (const exception& e) { ASSERT_EQ(e.code(), CURLE_ABORTED_BY_CALLBACK); }
}

TEST(ASYNCPP_CURL, WebClientCookies) {
	auto req = http_request::make_get("https://www.google.de");
	auto resp = req.execute_sync();
	ASSERT_EQ(resp.status_code, 200);
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
