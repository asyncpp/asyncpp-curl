#include <asyncpp/curl/executor.h>
#include <asyncpp/curl/handle.h>
#include <future>

namespace asyncpp::curl {
	executor::executor() : m_multi{}, m_thread{}, m_mtx{}, m_exit{false}, m_queue{} {
		m_thread = std::thread([this]() { this->worker_thread(); });
	}

	executor::~executor() noexcept {
		m_exit.store(true);
		m_multi.wakeup();
		if (m_thread.joinable()) m_thread.join();
	}

	void executor::worker_thread() noexcept {
		dispatcher::current(this);
		while (true) {
			int still_running = 0;
			{
				std::unique_lock lck(m_mtx);
				m_multi.perform(&still_running);
				multi::event evt;
				while (m_multi.next_event(evt)) {
					if (evt.handle->m_done_callback) {
						m_queue.emplace([cb = evt.handle->m_done_callback, res = evt.result_code]() { cb(res); });
					}
					if (evt.code == multi::event_code::done) m_multi.remove_handle(*evt.handle);
				}
			}
			while (true) {
				auto fn = m_queue.pop();
				if (!fn) {
					if (m_exit && still_running == 0) return;
					break;
				}
				if ((*fn)) (*fn)();
			}
			auto timeout = m_multi.timeout().count();
			if (timeout == 0) continue;
			if (timeout < 0) timeout = 500;
			{
				std::unique_lock lck(m_mtx);
				if (!m_scheduled.empty()) {
					auto now = std::chrono::steady_clock::now();
					while (!m_scheduled.empty()) {
						auto elem = m_scheduled.begin();
						auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(elem->first - now).count();
						if (diff > 100) {
							if (timeout > diff) timeout = diff;
							break;
						}
						auto fn = std::move(elem->second);
						m_scheduled.erase(elem);
						lck.unlock();
						fn();
						lck.lock();
					}
				}
			}
			m_multi.poll({}, timeout, nullptr);
		}
		dispatcher::current(nullptr);
	}

	void executor::add_handle(handle& hdl) {
		if (m_thread.get_id() == std::this_thread::get_id()) {
			return m_multi.add_handle(hdl);
		} else {
			std::promise<void> p;
			auto future = p.get_future();
			this->push([this, &p, &hdl]() {
				try {
					m_multi.add_handle(hdl);
					p.set_value();
				} catch (...) { p.set_exception(std::current_exception()); }
			});
			return future.get();
		}
	}

	void executor::remove_handle(handle& hdl) {
		if (m_thread.get_id() == std::this_thread::get_id()) {
			return m_multi.remove_handle(hdl);
		} else {
			std::promise<void> p;
			auto future = p.get_future();
			this->push([this, &p, &hdl]() {
				try {
					m_multi.remove_handle(hdl);
					p.set_value();
				} catch (...) { p.set_exception(std::current_exception()); }
			});
			return future.get();
		}
	}

	void executor::exec_awaiter::await_suspend(coroutine_handle<> h) noexcept {
		m_coro = h;
		m_handle->set_donefunction([this](int result) {
			m_result = result;
			m_coro.resume();
		});
		m_multi->add_handle(*m_handle);
	}

	void executor::push(std::function<void()> fn) {
		m_queue.emplace(std::move(fn));
		if (m_thread.get_id() != std::this_thread::get_id()) m_multi.wakeup();
	}

	void executor::schedule(std::function<void()> fn, std::chrono::milliseconds timeout) {
		auto now = std::chrono::steady_clock::now();
		this->schedule(std::move(fn), now + timeout);
	}

	void executor::schedule(std::function<void()> fn, std::chrono::steady_clock::time_point time) {
		if (m_thread.get_id() == std::this_thread::get_id()) {
			m_scheduled.emplace(time, std::move(fn));
		} else {
			std::unique_lock<std::mutex> lck(m_mtx);
			m_scheduled.emplace(time, std::move(fn));
			lck.unlock();
			m_multi.wakeup();
		}
	}

	executor& executor::get_default() {
		static executor instance{};
		return instance;
	}
} // namespace asyncpp::curl