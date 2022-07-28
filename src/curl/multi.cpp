#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/handle.h>
#include <asyncpp/curl/multi.h>
#include <cstring>
#include <curl/curl.h>
#include <curl/multi.h>
#include <mutex>
#include <unistd.h>
#if LIBCURL_VERSION_NUM < 0x074400
#include <sys/eventfd.h>
#endif

namespace asyncpp::curl {
	multi::multi() : m_mtx{}, m_instance{nullptr}, m_wakeup{-1} {
		// TODO: Needs a mutex
		m_instance = curl_multi_init();
		if (!m_instance) throw std::runtime_error("failed to create curl handle");
#if LIBCURL_VERSION_NUM < 0x074400
		m_wakeup = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
		if (m_wakeup < 0) {
			curl_multi_cleanup(m_instance);
			throw std::runtime_error("failed to create eventfd");
		}
#endif
	}

	multi::~multi() noexcept {
		if (m_instance) curl_multi_cleanup(m_instance);
		if (m_wakeup >= 0) close(m_wakeup);
	}

	void multi::add_handle(handle& hdl) {
		std::scoped_lock lck{m_mtx, hdl.m_mtx};
		if (hdl.m_multi == this) return;
		if (hdl.m_multi != nullptr) throw std::logic_error("handle is already part of a multi");
		auto res = curl_multi_add_handle(m_instance, hdl.raw());
		if (res != CURLM_OK) throw exception{res, true};
		hdl.m_multi = this;
	}

	void multi::remove_handle(handle& hdl) {
		std::scoped_lock lck{m_mtx, hdl.m_mtx};
		if (hdl.m_multi == nullptr) return;
		if (hdl.m_multi != this) throw std::logic_error("attempt to remove curl_handle from wrong multi");
		auto res = curl_multi_remove_handle(m_instance, hdl.raw());
		if (res != CURLM_OK) throw exception{res, true};
		hdl.m_multi = nullptr;
	}

	std::chrono::milliseconds multi::timeout() {
		std::scoped_lock lck{m_mtx};
		long timeout = -1;
		auto res = curl_multi_timeout(m_instance, &timeout);
		if (res != CURLM_OK) throw exception{res, true};
		return std::chrono::milliseconds{timeout};
	}

	void multi::perform(int* still_running) {
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_perform(m_instance, still_running);
		if (res != CURLM_OK) throw exception{res, true};
	}

	void multi::wait(std::span<curl_waitfd> extra_fds, int timeout_ms, int* num_fds) {
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_wait(m_instance, extra_fds.data(), extra_fds.size(), timeout_ms, num_fds);
		if (res != CURLM_OK) throw exception{res, true};
	}

	void multi::poll(std::span<curl_waitfd> extra_fds, int timeout_ms, int* num_fds) {
#if LIBCURL_VERSION_NUM < 0x074400
		curl_waitfd extra[extra_fds.size() + 1];
		memcpy(extra, extra_fds.data(), extra_fds.size_bytes());
		extra[extra_fds.size()].fd = m_wakeup;
		extra[extra_fds.size()].events = CURL_WAIT_POLLIN;
		extra[extra_fds.size()].revents = 0;
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_wait(m_instance, extra, extra_fds.size() + 1, timeout_ms, num_fds);
		if (res != CURLM_OK) throw exception{res, true};
		uint64_t temp;
		auto unused = read(m_wakeup, &temp, sizeof(temp));
		static_cast<void>(unused); // We dont care about the result
#else
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_poll(m_instance, extra_fds.data(), extra_fds.size(), timeout_ms, num_fds);
		if (res != CURLM_OK) throw exception{res, true};
#endif
	}

	void multi::wakeup() {
#if LIBCURL_VERSION_NUM < 0x074400
		uint64_t t = 1;
		auto unused = write(m_wakeup, &t, sizeof(t));
		static_cast<void>(unused);
#else
		auto res = curl_multi_wakeup(m_instance);
		if (res != CURLM_OK) throw exception{res, true};
#endif
	}

	void multi::fdset(fd_set& read_set, fd_set& write_set, fd_set& exc_set, int& max_fd) {
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_fdset(m_instance, &read_set, &write_set, &exc_set, &max_fd);
		if (res != CURLM_OK) throw exception{res, true};
	}

	bool multi::next_event(event& evt) {
		std::scoped_lock lck{m_mtx};
		int msgs_in_queue = -1;
		auto ptr = curl_multi_info_read(m_instance, &msgs_in_queue);
		if (ptr) {
			evt.code = static_cast<event_code>(ptr->msg);
			evt.handle = handle::get_handle_from_raw(ptr->easy_handle);
			evt.result_code = ptr->data.result;
			return true;
		}
		return false;
	}

	void multi::set_option_long(int opt, long val) {
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_setopt(m_instance, static_cast<CURLMoption>(opt), val);
		if (res != CURLM_OK) throw exception{res, true};
	}

	void multi::set_option_ptr(int opt, const void* str) {
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_setopt(m_instance, static_cast<CURLMoption>(opt), str);
		if (res != CURLM_OK) throw exception{res, true};
	}

	void multi::set_option_bool(int opt, bool on) {
		std::scoped_lock lck{m_mtx};
		auto res = curl_multi_setopt(m_instance, static_cast<CURLMoption>(opt), static_cast<long>(on ? 1 : 0));
		if (res != CURLM_OK) throw exception{res, true};
	}

} // namespace asyncpp::curl