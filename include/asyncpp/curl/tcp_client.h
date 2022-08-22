#pragma once
#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/handle.h>
#include <asyncpp/detail/std_import.h>
#include <cstdint>
#include <mutex>

namespace asyncpp::curl {
	class executor;
	class tcp_client {
		executor& m_executor;
		handle m_handle;

		std::recursive_mutex m_mtx;
		bool m_is_connected;
		std::function<size_t(bool)> m_send_handler;
		std::function<size_t(bool)> m_recv_handler;

	public:
		/**
		 * \brief Construct a new tcp client
		 * \param e The executor to use for the connection
		 */
		tcp_client(executor& e);
		/** \brief Construct a new tcp client using the default executor */
		tcp_client();
		/** \brief Destructor */
		~tcp_client() noexcept;
		tcp_client(const tcp_client&) = delete;
		tcp_client& operator=(const tcp_client&) = delete;
		tcp_client(tcp_client&&) = delete;
		tcp_client& operator=(tcp_client&&) = delete;

		executor& executor() noexcept { return m_executor; }
		handle& handle() noexcept { return m_handle; }

		/**
		 * \brief Connect to the given remote server
		 * \param remote The remote server, can be an ip or domain name
		 * \param port The remote port
		 * \param ssl Use ssl for connecting
		 * \param cb Callback to invoke after the connection attempt was made
		 * \note Due to a bug in libcurl this is not actually async.
		 */
		void connect(std::string remote, uint16_t port, bool ssl, std::function<void(int)> cb);

		/**
		 * \brief Connect to the given remote server
		 * \param remote The remote server, can be an ip or domain name
		 * \param port The remote port
		 * \param ssl Use ssl for connecting
		 * \return auto A awaitable type
		 * \note Due to a bug in libcurl this is not actually async.
		 */
		[[nodiscard]] auto connect(std::string remote, uint16_t port, bool ssl) {
			struct awaiter {
				tcp_client* const m_that;
				std::string m_remote;
				uint16_t const m_port;
				bool const m_ssl;
				int m_result{0};

				constexpr bool await_ready() const noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					m_that->connect(std::move(m_remote), m_port, m_ssl, [this, h](int code) {
						m_result = code;
						h.resume();
					});
				}
				constexpr void await_resume() const {
					if (m_result != 0) throw exception(m_result, false);
				}
			};
			return awaiter{this, remote, port, ssl};
		}

		/**
		 * \brief Close the currently active connection
		 * \param cb Invoked after the connection is closed
		 */
		void disconnect(std::function<void()> cb);

		/**
		 * \brief Disconnect from the server. This will reset the contained handle.
		 * \return auto A awaitable type
		 */
		[[nodiscard]] auto disconnect() {
			struct awaiter {
				tcp_client* const m_that;
				constexpr bool await_ready() const noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					m_that->disconnect([h]() { h.resume(); });
				}
				constexpr void await_resume() const noexcept {}
			};
			return awaiter{this};
		}

		/**
		 * \brief Send data to the remote host.
		 * \param buffer Pointer to the data to send
		 * \param size Size of the data to send in bytes.
		 * \param cb Callback to be invoke with the result. The callback is passed the number of bytes transmitted or 0 if the connection was closed.
		 */
		void send(const void* buffer, size_t size, std::function<void(size_t)> cb);

		/**
		 * \brief Send data to the remote host.
		 * \param buffer Pointer to the data to send
		 * \param size Size of the data to send in bytes.
		 * \return auto An awaitable for the send operation. The return value indicates the number of bytes transmitted or 0 if the connection was closed.
		 */
		[[nodiscard]] auto send(const void* buffer, size_t size) {
			struct awaiter {
				tcp_client* const m_that;
				const void* const m_buffer;
				const size_t m_size;
				size_t m_result{};
				constexpr bool await_ready() const noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					m_that->send(m_buffer, m_size, [this, h](size_t res) {
						m_result = res;
						h.resume();
					});
				}
				constexpr size_t await_resume() const noexcept { return m_result; }
			};
			return awaiter{this, buffer, size};
		}

		/**
		 * \brief Send all provided data to the remote host.
		 * \param buffer Pointer to the data to send
		 * \param size Size of the data to send in bytes.
		 * \param cb Callback to be invoke with the result. The callback is passed the number of bytes transmitted or 0 if the connection was closed.
		 * \note This differs from send() and will not call the callback until either the connection is closed or all data has been sent.
		 */
		void send_all(const void* buffer, size_t size, std::function<void(size_t)> cb);

		/**
		 * \brief Send all provided data to the remote host.
		 * \param buffer Pointer to the data to send
		 * \param size Size of the data to send in bytes.
		 * \return auto An awaitable for the send_all operation. The return value indicates the number of bytes transmitted or 0 if the connection was closed.
		 * \note This differs from send() and will not return until either the connection is closed or all data has been sent.
		 */
		[[nodiscard]] auto send_all(const void* buffer, size_t size) {
			struct awaiter {
				tcp_client* const m_that;
				const void* const m_buffer;
				const size_t m_size;
				size_t m_result{};
				constexpr bool await_ready() const noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					m_that->send_all(m_buffer, m_size, [this, h](size_t res) {
						m_result = res;
						h.resume();
					});
				}
				constexpr size_t await_resume() const noexcept { return m_result; }
			};
			return awaiter{this, buffer, size};
		}

		/**
		 * \brief Receive some data from the remote host.
		 * \param buffer Pointer to the data to store received data at
		 * \param size Size of the buffer to store data at
		 * \param cb Callback to get invoked once data is received. The size of the received data is passed, or zero if the connection was closed.
		 */
		void recv(void* buffer, size_t size, std::function<void(size_t)> cb);

		/**
		 * \brief Receive some data from the remote host.
		 * \param buffer Pointer to the data to store received data at
		 * \param size Size of the buffer to store data at
		 * \return auto An awaitable for the recv operation. The return value indicates the size of the received data, or zero if the connection was closed.
		 */
		[[nodiscard]] auto recv(void* buffer, size_t size) {
			struct awaiter {
				tcp_client* const m_that;
				void* const m_buffer;
				const size_t m_size;
				size_t m_result{};
				constexpr bool await_ready() const noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					m_that->recv(m_buffer, m_size, [this, h](size_t res) {
						m_result = res;
						h.resume();
					});
				}
				constexpr size_t await_resume() const noexcept { return m_result; }
			};
			return awaiter{this, buffer, size};
		}

		/**
		 * \brief Receive the specified amount of data from the remote host.
		 * \param buffer Pointer to the data to store received data at
		 * \param size Size of the buffer to store data at
		 * \param cb Callback to get invoked once data is received. The return value indicates the size of the received data, or zero if the connection was closed.
		 * \note This differs from recv() and will not return until either the connection is closed or all data has been received.
		 */
		void recv_all(void* buffer, size_t size, std::function<void(size_t)> cb);

		/**
		 * \brief Receive the specified amount of data from the remote host.
		 * \param buffer Pointer to the data to store received data at
		 * \param size Size of the buffer to store data at
		 * \return auto An awaitable for the recv_all operation. The return value indicates the size of the received data, or zero if the connection was closed.
		 * \note This differs from recv() and will not return until either the connection is closed or all data has been received.
		 */
		[[nodiscard]] auto recv_all(void* buffer, size_t size) {
			struct awaiter {
				tcp_client* const m_that;
				void* const m_buffer;
				const size_t m_size;
				size_t m_result{};
				constexpr bool await_ready() const noexcept { return false; }
				void await_suspend(coroutine_handle<> h) noexcept {
					m_that->recv_all(m_buffer, m_size, [this, h](size_t res) {
						m_result = res;
						h.resume();
					});
				}
				constexpr size_t await_resume() const noexcept { return m_result; }
			};
			return awaiter{this, buffer, size};
		}
	};
} // namespace asyncpp::curl