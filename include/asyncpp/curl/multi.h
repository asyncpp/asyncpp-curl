#pragma once
#include <chrono>
#include <cstddef>
#include <mutex>
#include <span>
#ifdef __linux__
#include <sys/select.h>
#elif _WIN32
#include <winsock.h>
#endif

struct curl_waitfd;
namespace asyncpp::curl {
	class handle;
	class multi {
		std::recursive_mutex m_mtx;
		void* m_instance;
		int m_wakeup; // eventfd used to implement poll & wakeup on pre 7.68.0
	public:
		multi();
		~multi() noexcept;
		multi(const multi&) = delete;
		multi& operator=(const multi&) = delete;
		multi(multi&&) = delete;
		multi& operator=(multi&&) = delete;

		operator bool() const noexcept { return m_instance != nullptr; }
		bool operator!() const noexcept { return m_instance == nullptr; }

		void* raw() const noexcept { return m_instance; }

		void add_handle(curl::handle& hdl);
		void remove_handle(curl::handle& hdl);

		std::chrono::milliseconds timeout();
		void perform(int* still_running);
		void wait(std::span<curl_waitfd> extra_fds, int timeout_ms, int* num_fds);
		void poll(std::span<curl_waitfd> extra_fds, int timeout_ms, int* num_fds);
		void wakeup();
		void fdset(fd_set& read_set, fd_set& write_set, fd_set& exc_set, int& max_fd);

		enum class event_code { done = 1 };
		struct event {
			event_code code;
			curl::handle* handle;
			int result_code;
		};
		bool next_event(event& evt);

		void set_option_long(int opt, long val);
		void set_option_ptr(int opt, const void* str);
		void set_option_bool(int opt, bool on);
	};
} // namespace asyncpp::curl
