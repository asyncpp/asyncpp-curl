#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/executor.h>
#include <asyncpp/curl/handle.h>
#include <asyncpp/curl/slist.h>
#include <asyncpp/curl/webclient.h>
#include <asyncpp/detail/std_import.h>
#include <curl/curl.h>
#include <ostream>
#include <stdexcept>
#include <variant>

namespace asyncpp::curl {

	http_request http_request::make_get(uri url) { return http_request{.request_method = "GET", .url = url}; }

	http_request http_request::make_head(uri url) { return http_request{.request_method = "HEAD", .url = url}; }

	http_request http_request::make_post(uri url, body_provider_t body) {
		return http_request{.request_method = "POST", .url = url, .body_provider = std::move(body)};
	}

	http_request http_request::make_patch(uri url, body_provider_t body) {
		return http_request{.request_method = "PATCH", .url = url, .body_provider = std::move(body)};
	}

	http_request http_request::make_put(uri url, body_provider_t body) {
		return http_request{.request_method = "PUT", .url = url, .body_provider = std::move(body)};
	}

	http_request http_request::make_delete(uri url) { return http_request{.request_method = "DELETE", .url = url}; }

	namespace {
		std::string_view trim(std::string_view str) {
			auto pos = str.find_first_not_of(" \t\n\v\f\r");
			if (pos == std::string::npos) pos = str.size();
			str.remove_prefix(pos);
			pos = str.find_last_not_of(" \t\n\v\f\r");
			if (pos == std::string::npos) return "";
			return str.substr(0, pos + 1);
		}

		auto make_header_cb(http_response& response) {
			return [&response](char* buffer, size_t size) mutable -> size_t {
				if (size == 0 || size == 1) return size;
				try {
					std::string_view line{buffer, size};
					line = trim(line);
					if (line.empty()) return size;
					// First line
					if (line.starts_with("HTTP/")) {
						auto pos = line.find(' ');
						if (pos == std::string::npos) return size;
						pos = line.find(' ', pos + 1);
						if (pos == std::string::npos) return size;
						response.status_message = trim(line.substr(pos + 1));
						response.headers.clear();
					} else {
						auto pos = line.find(':');
						if (pos == std::string::npos) {
							response.headers.emplace(line, "");
						} else {
							response.headers.emplace(trim(line.substr(0, pos)), trim(line.substr(pos + 1)));
						}
					}
				} catch (...) {
					return 0; // Write error
				}
				return size;
			};
		}

		void set_write_cb(handle& hdl, http_response& resp, http_response::body_storage_t body_store_method) {
			if (std::holds_alternative<http_response::ignore_body>(body_store_method)) {
				hdl.set_writefunction([](char*, size_t size) -> size_t { return size; });
			} else if (std::holds_alternative<http_response::inline_body>(body_store_method)) {
				hdl.set_writestring(resp.body);
			} else if (std::holds_alternative<std::string*>(body_store_method)) {
				hdl.set_writestring(*std::get<std::string*>(body_store_method));
			} else if (std::holds_alternative<std::ostream*>(body_store_method)) {
				hdl.set_writestream(*std::get<std::ostream*>(body_store_method));
			} else if (std::holds_alternative<std::function<size_t(char*, size_t)>>(body_store_method)) {
				hdl.set_writefunction(std::move(std::get<std::function<size_t(char*, size_t)>>(body_store_method)));
			} else
				throw std::logic_error("invalide variant");
		}

		void set_read_cb(handle& hdl, http_request::body_provider_t body_provider) {
			if (std::holds_alternative<http_request::no_body>(body_provider)) {
				hdl.set_readfunction([](char*, size_t) -> size_t { return 0; });
			} else if (std::holds_alternative<const std::string*>(body_provider)) {
				hdl.set_option_bool(CURLOPT_POST, true);
				hdl.set_option_long(CURLOPT_POSTFIELDSIZE_LARGE, std::get<const std::string*>(body_provider)->size());
				hdl.set_option_ptr(CURLOPT_POSTFIELDS, std::get<const std::string*>(body_provider)->data());
			} else if (std::holds_alternative<std::istream*>(body_provider)) {
				hdl.set_option_bool(CURLOPT_POST, true);
				hdl.set_readstream(*std::get<std::istream*>(body_provider));
			} else if (std::holds_alternative<std::function<size_t(char*, size_t)>>(body_provider)) {
				hdl.set_option_bool(CURLOPT_POST, true);
				hdl.set_readfunction(std::move(std::get<std::function<size_t(char*, size_t)>>(body_provider)));
			} else
				throw std::logic_error("invalide variant");
		}

		void prepare_handle(handle& hdl, const http_request& req, http_response& resp, http_response::body_storage_t body_store_method) {
			// Prepare response handling
			hdl.set_headerfunction(make_header_cb(resp));
			set_write_cb(hdl, resp, std::move(body_store_method));

			hdl.set_option_string(CURLOPT_CUSTOMREQUEST, req.request_method.c_str());
			auto string_url = req.url.to_string();
			hdl.set_option_string(CURLOPT_URL, string_url.c_str());
			slist out_headers{};
			for (auto& e : req.headers) {
				std::string hdr{e.first};
				if (e.second.empty())
					hdr += ";";
				else
					hdr += ": " + e.second;
				out_headers.append(hdr.c_str());
			}
			hdl.set_headers(std::move(out_headers));
			set_read_cb(hdl, req.body_provider);
			hdl.set_follow_location(req.follow_redirects);
			hdl.set_verbose(req.verbose);
			hdl.set_option_long(CURLOPT_TIMEOUT_MS, req.timeout.count());
			hdl.set_option_long(CURLOPT_CONNECTTIMEOUT_MS, req.timeout_connect.count());
			if (auto user = req.url.auth(); !user.empty()) {
				auto pos = user.find(":");
				if (pos != std::string::npos) {
					hdl.set_option_string(CURLOPT_PASSWORD, user.substr(pos + 1).c_str());
					user.resize(pos);
				}
				hdl.set_option_string(CURLOPT_USERNAME, user.c_str());
			}
			// Wipe the cookie chain of existing cookies
			hdl.set_option_string(CURLOPT_COOKIEFILE, "");
			for (auto& e : req.cookies) {
				hdl.set_option_string(CURLOPT_COOKIELIST, e.to_string().c_str());
			}
		}
	} // namespace

	http_response http_request::execute_sync(http_response::body_storage_t body_store_method) {
		http_response response{};
		handle hdl{};
		prepare_handle(hdl, *this, response, std::move(body_store_method));

		if (configure_hook) configure_hook(hdl);
		hdl.perform();
		if (result_hook) result_hook(hdl);

		response.status_code = hdl.get_response_code();
		auto cookies = hdl.get_info_slist(CURLINFO_COOKIELIST);
		for (auto e : cookies) {
			response.cookies.emplace_back(e);
		}
		return response;
	}

	struct http_request::execute_awaiter::data {
		data(executor* exec, http_request* req, std::stop_token st) : m_exec(exec, &m_handle, std::move(st)), m_request(req) {}

		executor::exec_awaiter m_exec;
		handle m_handle{};
		http_request* m_request{};
		http_response m_response{};
	};

	http_request::execute_awaiter::execute_awaiter(http_request& req, http_response::body_storage_t storage, executor* executor, std::stop_token st) {
		m_impl = new data(executor ? executor : &executor::get_default(), &req, std::move(st));
		prepare_handle(m_impl->m_handle, req, m_impl->m_response, std::move(storage));
	}

	http_request::execute_awaiter::~execute_awaiter() {
		if (m_impl) delete m_impl;
	}

	void http_request::execute_awaiter::await_suspend(coroutine_handle<> h) noexcept {
		if (m_impl->m_request->configure_hook) m_impl->m_request->configure_hook(m_impl->m_handle);
		m_impl->m_exec.await_suspend(h);
	}

	http_response http_request::execute_awaiter::await_resume() const {
		auto res = m_impl->m_exec.await_resume();
		if (m_impl->m_request->result_hook) m_impl->m_request->result_hook(m_impl->m_handle);
		if (res != CURLE_OK) throw exception(res, false);
		m_impl->m_response.status_code = m_impl->m_handle.get_response_code();
		return std::move(m_impl->m_response);
	}

} // namespace asyncpp::curl
