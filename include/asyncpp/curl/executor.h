#pragma once
#include <asyncpp/curl/multi.h>
#include <asyncpp/detail/std_import.h>
#include <asyncpp/dispatcher.h>
#include <asyncpp/threadsafe_queue.h>

#include <atomic>
#include <chrono>
#include <future>
#include <map>
#include <mutex>
#include <set>
#include <stop_token>
#include <string>
#include <thread>

namespace asyncpp::curl {
	class handle;
	/**
	 * \brief Curl Executor class, implements a dispatcher on top of curl_multi_*.
	 */
	class executor : public dispatcher {
		multi m_multi;
		std::thread m_thread;
		std::mutex m_mtx;
		std::atomic<bool> m_exit;
		threadsafe_queue<std::function<void()>> m_queue;
		std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> m_scheduled;
		std::set<handle*> m_connect_only_handles;

		void worker_thread() noexcept;

	public:
		/**
		 * \brief Construct a new executor object
		 */
		executor();
		/**
		 * \brief Destroy the executor object
		 */
		~executor() noexcept;
		executor(const executor&) = delete;
		executor& operator=(const executor&) = delete;
		executor(executor&&) = delete;
		executor& operator=(executor&&) = delete;

		/**
		 * \brief Add an easy handle to this executor.
		 * \param hdl The handle to add
		 * \note Due to a bug in libcurl a connect_only handle can not use asynchronous connect and has to use perform() instead.
		 * \note Adding it to the executor is still supported and will poll the handle for readiness, causeing the read/write callback to be invoked.
		 */
		void add_handle(handle& hdl);
		/**
		 * \brief Remove an easy handle from this executor.
		 * \param hdl The handle to remove
		 */
		void remove_handle(handle& hdl);

		/** \brief coroutine awaiter for an easy transfer */
		struct exec_awaiter {
			executor* const m_parent;
			handle* const m_handle;
			struct stop_callback {
				executor* const m_parent;
				handle* const m_handle;
				void operator()();
			};
			std::stop_callback<stop_callback> m_callback;
			int m_result{0};

			exec_awaiter(executor* exec, handle* hdl, std::stop_token st)
				: m_parent(exec), m_handle(hdl), m_callback(std::move(st), stop_callback{exec, hdl}) {}

			constexpr bool await_ready() const noexcept { return false; }
			void await_suspend(coroutine_handle<> h) noexcept;
			constexpr int await_resume() const noexcept { return m_result; }
		};

		/**
		 * \brief Return a awaitable that suspends till the handle is finished.
		 * \param hdl The handle to await
		 * \return An awaitable
		 */
		exec_awaiter exec(handle& hdl, std::stop_token st = {});

		/**
		 * \brief Push a invocable to be executed on the executor thread.
		 * \param fn Invocable to call
		 */
		void push(std::function<void()> fn) override;
		/**
		 * \brief Schedule an invocable in a certain time from now.
		 * \param fn Invocable to execute on the executor thread
		 * \param timeout Timeout to execute the invocable at
		 */
		void schedule(std::function<void()> fn, std::chrono::milliseconds timeout);
		/**
		 * \brief Schedule an invocable at a certain timepoint.
		 * \param fn Invocable to execute on the executor thread
		 * \param time Timestamp at which to execute the invocable
		 */
		void schedule(std::function<void()> fn, std::chrono::steady_clock::time_point time);

		/**
		 * \brief Run an invocable on the executor thread and return the result.
		 * \param fn Invocable to call
		 */
		template<typename FN>
		std::invoke_result_t<FN> push_wait(FN&& fn) {
			if (m_thread.get_id() == std::this_thread::get_id()) {
				return fn();
			} else {
				std::promise<std::invoke_result_t<FN>> promise;
				auto result = promise.get_future();
				this->push([&promise, &fn]() {
					try {
						if constexpr (std::is_same_v<std::invoke_result_t<FN>, void>) {
							fn();
							promise.set_value();
						} else
							promise.set_value(fn());
					} catch (...) { promise.set_exception(std::current_exception()); }
				});
				return result.get();
			}
		}

		/**
		 * \brief Wake the underlying multi handle
		 */
		void wakeup();

		/**
		 * \brief Get a global default executor
		 * \return A global executor instance
		 * \note Do not keep references to this executor past the end of main because the destruction order is not predictable.
		 */
		static executor& get_default();
	};
} // namespace asyncpp::curl
