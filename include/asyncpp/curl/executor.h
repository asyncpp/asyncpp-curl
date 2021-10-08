#pragma once
#include <asyncpp/curl/multi.h>
#include <asyncpp/detail/std_import.h>
#include <asyncpp/dispatcher.h>
#include <asyncpp/threadsafe_queue.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace asyncpp::curl {
	class handle;
	class executor : public dispatcher {
		multi m_multi;
		std::thread m_thread;
		std::mutex m_mtx;
		std::atomic<bool> m_exit;
		threadsafe_queue<std::function<void()>> m_queue;
		std::vector<std::pair<std::chrono::steady_clock::time_point, std::function<void()>>> m_scheduled;

	public:
		executor();
		executor(const executor&) = delete;
		executor& operator=(const executor&) = delete;
		executor(executor&&) = delete;
		executor& operator=(executor&&) = delete;
		~executor() noexcept;

		void add_handle(handle& hdl);
		void remove_handle(handle& hdl);

		struct exec_awaiter {
			executor* m_multi;
			handle* m_handle;
			coroutine_handle<> m_coro{};
			int m_result{0};

			bool await_ready() noexcept;
			void await_suspend(coroutine_handle<> h) noexcept;
			int await_resume() noexcept;
		};
		exec_awaiter exec(handle& hdl) { return exec_awaiter{this, &hdl}; }

		void push(std::function<void()> fn) override;
		void schedule(std::function<void()> fn, std::chrono::milliseconds timeout);

		static executor& get_default();
	};
} // namespace asyncpp::curl