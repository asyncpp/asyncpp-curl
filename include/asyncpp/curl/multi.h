#pragma once
#include <cstddef>
#include <span>

namespace asyncpp::curl {
	class handle;
	class multi {
		void* m_instance;
		int m_wakeup; // eventfd used to implement poll & wakeup on pre 7.68.0
	public:
		multi();
		multi(const multi&) = delete;
		multi& operator=(const multi&) = delete;
		multi(multi&&) = delete;
		multi& operator=(multi&&) = delete;
		~multi() noexcept;

		operator bool() const noexcept { return m_instance != nullptr; }
		bool operator!() const noexcept { return m_instance == nullptr; }

		void* raw() const noexcept { return m_instance; }

		void add_handle(curl::handle& hdl);
		void remove_handle(curl::handle& hdl);

		long timeout();
		void perform(int* still_running);
		void wait(std::span<int> extra_fds, int timeout_ms, int* num_fds);
		void poll(std::span<int> extra_fds, int timeout_ms, int* num_fds);
		void wakeup();

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