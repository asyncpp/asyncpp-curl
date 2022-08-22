#include <asyncpp/curl/tcp_client.h>
#include <asyncpp/launch.h>
#include <asyncpp/sync_wait.h>
#include <asyncpp/task.h>

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

using namespace asyncpp::curl;
using namespace asyncpp;

#define COASSERT_EQ(a, b)                                                                                                                                      \
	{                                                                                                                                                          \
		bool failed{true};                                                                                                                                     \
		[&]() {                                                                                                                                                \
			ASSERT_EQ(a, b);                                                                                                                                   \
			failed = false;                                                                                                                                    \
		}();                                                                                                                                                   \
		if (failed) co_return;                                                                                                                                 \
	}

TEST(ASYNCPP_CURL, TcpClient) {
	as_promise([]() -> task<void> {
		std::string str = "Hello World\n";
		tcp_client client;
		co_await client.connect("tcpbin.com", 4242, false);
		auto written = co_await client.send_all(str.data(), str.size());
		COASSERT_EQ(written, 12);
		char buf[128]{};
		auto read = co_await client.recv(buf, 128);
		COASSERT_EQ(read, str.size());
		std::string_view read_str(buf, str.size());
		COASSERT_EQ(str, read_str);
		co_await client.disconnect();
		co_return;
	}())
		.get();
}

TEST(ASYNCPP_CURL, TcpClientAsyncRead) {
	as_promise([]() -> task<void> {
		async_launch_scope scope;
		static const std::string str = "Hello World\n";

		tcp_client client;
		co_await client.connect("tcpbin.com", 4242, false);

		bool did_receive = false;
		scope.launch([&client, &did_receive]() -> task<void> {
			char buf[12]{};
			auto read = co_await client.recv(buf, 12);
			COASSERT_EQ(read, str.size());
			std::string_view read_str(buf, str.size());
			COASSERT_EQ(str, read_str);
			did_receive = true;
		}());
		COASSERT_EQ(did_receive, 0);

		auto written = co_await client.send_all(str.data(), str.size());
		COASSERT_EQ(written, 12);

		co_await scope.join();
		COASSERT_EQ(did_receive, 1);
		did_receive = false;
		scope.launch([&client, &did_receive]() -> task<void> {
			char buf[12]{};
			auto read = co_await client.recv(buf, 12);
			COASSERT_EQ(read, 0);
			did_receive = true;
		}());
		COASSERT_EQ(did_receive, 0);
		co_await client.disconnect();
		co_await scope.join();

		char buf[12]{};
		auto read = co_await client.recv(buf, 12);
		COASSERT_EQ(read, 0);

		co_return;
	}())
		.get();
}
