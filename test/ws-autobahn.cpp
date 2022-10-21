#include <asyncpp/sync_wait.h>
#include <asyncpp/task.h>
#include <future>
#include <iostream>

#include <asyncpp/curl/websocket.h>
#include <asyncpp/curl/executor.h>
#include <asyncpp/launch.h>
#include <asyncpp/promise.h>
#include <asyncpp/defer.h>
#include <variant>

using namespace asyncpp;
using namespace asyncpp::curl;

promise<std::pair<std::string, bool>> read_message(websocket& ws) {
	promise<std::pair<std::string, bool>> res;
	ws.set_on_open([res](int code) mutable {
		if(code != 0)
			res.try_reject<exception>(code);
	});
	ws.set_on_message([res, &ws](websocket::buffer data, bool binary) mutable {
        std::pair<std::string, bool> p;
		p.first.append(reinterpret_cast<const char*>(data.data()), data.size());
		p.second = binary;
        ws.set_on_message({});
        ws.set_on_close({});
		res.try_fulfill(std::move(p));
	});
    ws.set_on_close([res, &ws](int, std::string_view) mutable {
        res.try_reject<std::runtime_error>("disconnected");
    });
	return res;
}

task<int> async_main(int argc, const char** argv) {
	if (argc < 2) {
		std::cerr << argv[0] << " <autobahn server> [AppName]" << std::endl;
		co_return -1;
	}

	std::string base = argv[1];
	std::string app_name = argc > 2 ? argv[2] : "asyncpp-curl";

	// Get testcase count
	size_t ncases = 0;
	{
		websocket socket;
        auto msg = read_message(socket);
		socket.connect(uri(base + "/getCaseCount"));
        auto res = co_await msg;
        ncases = std::stoull(res.first);
	}
    std::cout << "Number of cases: " << ncases << std::endl;
	for(size_t i = 1; i < ncases; i++) {
		websocket socket;
		socket.set_on_open([i](int code){
			std::cout << "Test case " << i << " started..." << std::endl;
		});
		socket.set_on_message([&socket](websocket::buffer data, bool binary) {
			socket.send(data, binary, [](bool){});
		});
		promise<bool> res;
		socket.set_on_close([i, ncases, res](uint16_t code, std::string_view reason) mutable {
			std::cout << "Test case " << i << "/" << ncases << " finished... (code=" << code << ", reason=\"" << reason << "\")" << std::endl;
			res.fulfill({});
		});
		socket.connect(uri(base + "/runCase?case=" + std::to_string(i) + "&agent=asyncpp-curl"));
		co_await res;
	}
	{
		websocket socket;
		socket.connect(uri(base + "/updateReports?agent=asyncpp-curl"));
	}

	co_return 0;
}

int main(int argc, const char** argv) { return as_promise(async_main(argc, argv)).get(); }