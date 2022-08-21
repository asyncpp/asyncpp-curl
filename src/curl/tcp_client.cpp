#include <asyncpp/curl/executor.h>
#include <asyncpp/curl/tcp_client.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <curl/curl.h>
#include <exception>
#include <string>

namespace asyncpp::curl {
	tcp_client::tcp_client(curl::executor& e) : m_executor{e}, m_handle{}, m_is_connected{false} {}

	tcp_client::tcp_client() : tcp_client(executor::get_default()) {}

	tcp_client::~tcp_client() noexcept {}

	void tcp_client::connect(std::string remote, uint16_t port, bool ssl, std::function<void(int)> cb) {
		std::unique_lock lck{m_mtx};
		m_handle.set_option_bool(CURLOPT_CONNECT_ONLY, true);
		m_handle.set_option_bool(CURLOPT_FRESH_CONNECT, true);
		m_handle.set_readfunction([this](char* buf, size_t size) -> size_t {
			assert(buf == nullptr && size == 0);
			std::unique_lock lck{m_mtx};
			auto res = CURL_READFUNC_PAUSE;
			if (m_send_handler) res = m_send_handler(false);
			if (CURL_READFUNC_PAUSE) m_send_handler = {};
			return res;
		});
		m_handle.set_writefunction([this](char* buf, size_t size) -> size_t {
			assert(buf == nullptr && size == 0);
			std::unique_lock lck{m_mtx};
			auto res = CURL_READFUNC_PAUSE;
			if (m_recv_handler) res = m_recv_handler(false);
			if (CURL_READFUNC_PAUSE) m_recv_handler = {};
			return res;
		});
		m_handle.set_url((ssl ? "https://" : "http://") + remote + ":" + std::to_string(port));
		m_handle.set_donefunction([this, cb = std::move(cb)](int res) {
			if(res == CURLE_OK) {
				std::unique_lock lck{m_mtx};
				m_is_connected = true;
				if (m_handle.is_verbose()) printf("* curl::tcp_client connected\n");
			}
			cb(res);
		});
		m_handle.pause(CURLPAUSE_ALL);
		if (m_handle.is_verbose()) printf("* curl::tcp_client connecting to %s:%d ssl=%s\n", remote.c_str(), port, (ssl ? "true" : "false"));
		m_executor.add_handle(m_handle);
		try {
			lck.unlock();
			m_handle.perform();
		} catch (const std::exception& e) {
			if (m_handle.is_verbose()) printf("* curl::tcp_client failure connecting: %s\n", e.what());
			m_executor.remove_handle(m_handle);
			throw;
		}
	}

	void tcp_client::disconnect(std::function<void()> cb) {
		std::unique_lock lck{m_mtx};
		if (m_handle.is_verbose()) printf("* curl::tcp_client resetting handle to disconnect\n");
		if (m_recv_handler) m_recv_handler(true);
		if (m_send_handler) m_send_handler(true);
		m_send_handler = {};
		m_recv_handler = {};
		m_handle.reset();
		m_is_connected = false;
		lck.unlock();
		cb();
	}

	void tcp_client::send(const void* buffer, size_t size, std::function<void(size_t)> cb) {
		std::unique_lock lck{m_mtx};
		if (!m_is_connected) {
			lck.unlock();
			return cb(0);
		}
		// Try inline send
		auto res = m_handle.send(buffer, size);
		if (res != -1) {
			if (m_handle.is_verbose()) printf("* curl::tcp_client.send finished inline res=%ld\n", res);
			lck.unlock();
			return cb(res);
		}
		// No data available, set the callback and unpause the transfer (adds the connections to poll).
		m_send_handler = [this, buffer, size, cb](bool cancel) -> size_t {
			if (cancel) {
				m_executor.push([cb = std::move(cb)]() { cb(0); });
				return CURL_READFUNC_PAUSE;
			}
			auto res = m_handle.send(buffer, size);
			if (res == -1) return 0;
			// We push the actual callback invocation into the queue to make sure we don't end up erasing ourself
			// if the callback invokes send, or deadlock. An alternative would be implementing a function like type that
			// is safe to reassign while being executed, but this is way simpler and probably fast enough.
			m_executor.push([cb = std::move(cb), res]() { cb(res); });
			return CURL_READFUNC_PAUSE;
		};
		m_handle.unpause(CURLPAUSE_SEND);
	}

	void tcp_client::send_all(const void* buffer, size_t size, std::function<void(size_t)> cb) {
		std::unique_lock lck{m_mtx};
		if (!m_is_connected) {
			lck.unlock();
			return cb(0);
		}
		// Try inline send
		auto res = m_handle.send(buffer, size);
		if (res == 0 || static_cast<size_t>(res) == size) {
			if (m_handle.is_verbose()) printf("* curl::tcp_client.send_all finished inline res=%ld\n", res);
			lck.unlock();
			return cb(res);
		}
		auto u8ptr = reinterpret_cast<const uint8_t*>(buffer);
		size_t sent{(res < 0) ? 0u : static_cast<size_t>(res)};
		m_send_handler = [this, u8ptr, sent, size, cb](bool cancel) mutable -> size_t {
			if (cancel) {
				m_executor.push([cb = std::move(cb), sent]() { cb(sent); });
				return CURL_READFUNC_PAUSE;
			}
			auto res = m_handle.send(u8ptr + sent, size - sent);
			if (res == -1) return 0;
			sent += res;
			if (res == 0 || size == sent) {
				// See comment in send for details
				m_executor.push([cb = std::move(cb), sent]() { cb(sent); });
				return CURL_READFUNC_PAUSE;
			}
			return res;
		};
		m_handle.unpause(CURLPAUSE_SEND);
	}

	void tcp_client::recv(void* buffer, size_t size, std::function<void(size_t)> cb) {
		std::unique_lock lck{m_mtx};
		if (!m_is_connected) {
			lck.unlock();
			return cb(0);
		}
		// Try inline recv
		auto res = m_handle.recv(buffer, size);
		if (res != -1) {
			if (m_handle.is_verbose()) printf("* curl::tcp_client.recv finished inline res=%ld\n", res);
			lck.unlock();
			return cb(res);
		}
		m_recv_handler = [this, buffer, size, cb](bool cancel) -> size_t {
			if (cancel) {
				m_executor.push([cb = std::move(cb)]() { cb(0); });
				return CURL_WRITEFUNC_PAUSE;
			}
			auto res = m_handle.recv(buffer, size);
			if (res == -1) return 0;
			// See comment in send for details
			m_executor.push([cb = std::move(cb), res]() { cb(res); });
			return CURL_WRITEFUNC_PAUSE;
		};
		m_handle.unpause(CURLPAUSE_RECV);
	}

	void tcp_client::recv_all(void* buffer, size_t size, std::function<void(size_t)> cb) {
		std::unique_lock lck{m_mtx};
		if (!m_is_connected) {
			lck.unlock();
			return cb(0);
		}
		// Try inline send
		auto res = m_handle.recv(buffer, size);
		if (res == 0 || static_cast<size_t>(res) == size) {
			if (m_handle.is_verbose()) printf("* curl::tcp_client.recv_all finished inline res=%ld\n", res);
			lck.unlock();
			return cb(res);
		}
		auto u8ptr = reinterpret_cast<uint8_t*>(buffer);
		size_t read{(res < 0) ? 0u : static_cast<size_t>(res)};
		m_recv_handler = [this, u8ptr, read, size, cb](bool cancel) mutable -> size_t {
			if (cancel) {
				m_executor.push([cb = std::move(cb), read]() { cb(read); });
				return CURL_WRITEFUNC_PAUSE;
			}
			auto res = m_handle.recv(u8ptr + read, size - read);
			if (res == -1) return 0;
			read += res;
			if (res == 0 || size == read) {
				// See comment in send for details
				m_executor.push([cb = std::move(cb), read]() { cb(read); });
				return CURL_WRITEFUNC_PAUSE;
			}
			return res;
		};
		m_handle.unpause(CURLPAUSE_RECV);
	}
} // namespace asyncpp::curl