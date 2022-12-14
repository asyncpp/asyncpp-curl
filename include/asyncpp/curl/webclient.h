#pragma once
#include <asyncpp/curl/cookie.h>
#include <asyncpp/curl/uri.h>
#include <asyncpp/curl/util.h>
#include <asyncpp/detail/std_import.h>
#include <cctype>
#include <chrono>
#include <exception>
#include <functional>
#include <map>
#include <stop_token>
#include <variant>

namespace asyncpp::curl {
	class executor;
	class handle;

	struct http_response {
		struct ignore_body {};
		struct inline_body {};
		using body_storage_t = std::variant<ignore_body, inline_body, std::string*, std::ostream*, std::function<size_t(char* ptr, size_t size)>>;

		/** \brief The HTTP status code returned from the transfer */
		int status_code;
		/** \brief The HTTP status message returned from the transfer */
		std::string status_message;
		/** \brief Headers received */
		std::multimap<std::string, std::string, case_insensitive_less> headers;
		/** \brief Cookies received/persisted */
		std::vector<cookie> cookies;
		/** \brief The response body if the store mode was inline_body */
		std::string body;
	};

	struct http_request {
		struct no_body {};
		using body_provider_t = std::variant<no_body, const std::string*, std::istream*, std::function<size_t(char* ptr, size_t size)>>;

		/** \brief Method to use for the request */
		std::string request_method{};
		/** \brief The url to request */
		uri url{};
		/** \brief Outgoing headers */
		std::multimap<std::string, std::string, case_insensitive_less> headers{};
		/** \brief Cookies to send along the request */
		std::vector<cookie> cookies;
		/** \brief Upload body policy */
		body_provider_t body_provider{};
		/** \brief Follow http redirects and return the last requests info */
		bool follow_redirects{true};
		/** \brief Enable verbose logging of curl */
		bool verbose{false};
		/** \brief Timeout for the entire operation, set to 0 to disable */
		std::chrono::milliseconds timeout{0};
		/** \brief Timeout for connecting (namelookup, proxy handling, connect), set to 0 to disable */
		std::chrono::milliseconds timeout_connect = std::chrono::seconds{30};
		/** \brief Hook executed right before performing the request. Can be used to set custom curl options if needed. */
		std::function<void(handle&)> configure_hook;
		/** \brief Hook executed right after performing the request. Can be used to read custom curl info if needed. */
		std::function<void(handle&)> result_hook;

		static http_request make_get(uri url);
		static http_request make_head(uri url);
		static http_request make_post(uri url, body_provider_t body = no_body{});
		static http_request make_patch(uri url, body_provider_t body = no_body{});
		static http_request make_put(uri url, body_provider_t body = no_body{});
		static http_request make_delete(uri url);

		http_response execute_sync(http_response::body_storage_t body_store_method = http_response::inline_body{});
		struct execute_awaiter {
			execute_awaiter(http_request& req, http_response::body_storage_t storage, executor* exec, std::stop_token st = {});
			~execute_awaiter();
			execute_awaiter(const execute_awaiter&) = delete;
			execute_awaiter(execute_awaiter&& other) : m_impl{other.m_impl} { other.m_impl = nullptr; }
			execute_awaiter& operator=(const execute_awaiter&) = delete;
			execute_awaiter& operator=(execute_awaiter&& other) {
				std::swap(m_impl, other.m_impl);
				return *this;
			}

			struct data;
			data* m_impl;

			constexpr bool await_ready() const noexcept { return false; }
			void await_suspend(coroutine_handle<> h) noexcept;
			http_response await_resume() const;
		};
		execute_awaiter execute_async(http_response::body_storage_t body_store_method = http_response::inline_body{}) {
			return execute_awaiter{*this, std::move(body_store_method), nullptr};
		}
		execute_awaiter execute_async(http_response::body_storage_t body_store_method, executor& exec) {
			return execute_awaiter{*this, std::move(body_store_method), &exec};
		}
		execute_awaiter execute_async(std::stop_token st, http_response::body_storage_t body_store_method = http_response::inline_body{}) {
			return execute_awaiter{*this, std::move(body_store_method), nullptr, std::move(st)};
		}
		execute_awaiter execute_async(std::stop_token st, http_response::body_storage_t body_store_method, executor& exec) {
			return execute_awaiter{*this, std::move(body_store_method), &exec, std::move(st)};
		}
	};
} // namespace asyncpp::curl
