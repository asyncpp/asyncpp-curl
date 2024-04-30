#include <iostream>

#include <asyncpp/async_main.h>
#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/uri.h>
#include <asyncpp/curl/websocket.h>
#include <asyncpp/promise.h>

using namespace asyncpp;
using namespace asyncpp::curl;

promise<std::pair<std::string, bool>> read_message(websocket* ws) {
	dispatcher* const main_dp = dispatcher::current();
	promise<std::pair<std::string, bool>> res;
	ws->set_on_open([=](int code) mutable {
		main_dp->push([code, res]() mutable {
			if (code != 0) res.try_reject<exception>(code);
		});
	});
	ws->set_on_message([=](websocket::buffer data, bool binary) mutable {
		std::pair<std::string, bool> p;
		p.first.append(reinterpret_cast<const char*>(data.data()), data.size());
		p.second = binary;
		ws->set_on_message({});
		ws->set_on_close({});
		main_dp->push([p = std::move(p), res]() mutable { res.try_fulfill(std::move(p)); });
	});
	ws->set_on_close([=](int, std::string_view) mutable {
		ws->set_on_message({});
		ws->set_on_close({});
		main_dp->push([res]() mutable { res.try_reject<std::runtime_error>("disconnected"); });
	});
	return res;
}

task<int> async_main(int argc, const char** argv) {
	if (argc < 2) {
		std::cerr << argv[0] << " <autobahn server> [caseID]" << std::endl;
		co_return -1;
	}

	const std::string base = argv[1];
	const std::string app_name = "asyncpp-curl";
	dispatcher* const main_dp = dispatcher::current();

	// Get testcase count
	size_t ncases = argc > 2 ? std::stoull(argv[2]) : 0;
	if (ncases == 0) {
		websocket socket;
		auto msg = read_message(&socket);
		socket.connect(uri(base + "/getCaseCount"));
		auto res = co_await msg;
		ncases = std::stoull(res.first);
	}
	std::cout << "Number of cases: " << ncases << std::endl;
	for (size_t i = argc > 2 ? ncases : 1; i <= ncases; i++) {
		websocket socket;
		socket.set_on_open([i, ncases](int code) { std::cout << "Test case " << i << "/" << ncases << " ... " << std::flush; });
		socket.set_on_message([&socket](websocket::buffer data, bool binary) { socket.send(data, binary, [](bool) {}); });
		promise<void> res;
		socket.set_on_close([res, main_dp](uint16_t code, std::string_view reason) mutable {
			std::cout << "finished (code=" << code << ", reason=\"" << reason << "\")" << std::endl;
			main_dp->push([res]() mutable { res.fulfill(); });
		});
		socket.connect(uri(base + "/runCase?case=" + std::to_string(i) + "&agent=" + app_name));
		co_await res;
	}
	{
		websocket socket;
		socket.connect(uri(base + "/updateReports?agent=" + app_name));
	}

	co_return 0;
}
