#include <asyncpp/curl/executor.h>
#include <asyncpp/curl/handle.h>
#include <curl/curl.h>
#include <curl/multi.h>
#include <future>
#include <stdexcept>

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
		std::vector<curl_waitfd> fds;
		std::vector<handle*> handles;
		while (true) {
			int still_running = 0;
			{
				std::unique_lock lck(m_mtx);
				m_multi.perform(&still_running);
				multi::event evt;
				while (m_multi.next_event(evt)) {
					std::unique_lock lck_hdl(evt.handle->m_mtx);
					if (evt.code == multi::event_code::done) {
						auto cb = std::exchange(evt.handle->m_done_callback, {});
						if (!evt.handle->is_connect_only()) m_multi.remove_handle(*evt.handle);
						if (cb) m_queue.emplace([cb = std::move(cb), res = evt.result_code]() { cb(res); });
					}
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
			{
				auto num_handles = m_connect_only_handles.size();
				fds.resize(num_handles);
				handles.resize(num_handles);
				for (size_t i = 0; auto e : m_connect_only_handles) {
					auto fd = e->get_info_socket(CURLINFO_ACTIVESOCKET);
					fds[i].events = e->is_paused(CURLPAUSE_RECV) ? 0 : (CURL_WAIT_POLLIN | CURL_WAIT_POLLPRI);
					fds[i].events |= e->is_paused(CURLPAUSE_SEND) ? 0 : CURL_WAIT_POLLOUT;
					if (fd == std::numeric_limits<uint64_t>::max() || fds[i].events == 0) {
						num_handles--;
						continue;
					}
					fds[i].fd = fd;
					fds[i].revents = 0;
					handles[i] = e;
					i++;
				}
				int num_fds = 0;
				m_multi.poll({fds.data(), num_handles}, timeout, &num_fds);
				if (num_fds != 0) {
					for (size_t i = 0; i < num_handles; i++) {
						if (fds[i].revents & CURL_WAIT_POLLIN) {
							if (!handles[i]->m_write_callback || handles[i]->m_write_callback(nullptr, 0) == CURL_WRITEFUNC_PAUSE)
								handles[i]->pause(CURLPAUSE_RECV);
						} else if (fds[i].revents & CURL_WAIT_POLLOUT) {
							if (!handles[i]->m_read_callback || handles[i]->m_read_callback(nullptr, 0) == CURL_READFUNC_PAUSE)
								handles[i]->pause(CURLPAUSE_SEND);
						}
					}
				}
			}
		}
		dispatcher::current(nullptr);
	}

	void executor::add_handle(handle& hdl) {
		push_wait([this, &hdl]() {
			hdl.m_executor = this;
			if (hdl.is_connect_only())
				m_connect_only_handles.insert(&hdl);
			else
				m_multi.add_handle(hdl);
		});
	}

	void executor::remove_handle(handle& hdl) {
		push_wait([this, &hdl]() {
			hdl.m_executor = nullptr;
			if (hdl.is_connect_only())
				m_connect_only_handles.erase(&hdl);
			else
				m_multi.remove_handle(hdl);
		});
	}

	void executor::exec_awaiter::await_suspend(coroutine_handle<> h) noexcept {
		m_handle->set_donefunction([this, h](int result) {
			m_result = result;
			h.resume();
		});
		m_parent->add_handle(*m_handle);
	}

	void executor::exec_awaiter::stop_callback::operator()() {
		std::unique_lock lck{m_handle->m_mtx};
		auto cb = std::exchange(m_handle->m_done_callback, {});
		if (cb) {
			lck.unlock();
			m_parent->push([this, cb = std::move(cb)]() {
				std::unique_lock lck{m_handle->m_mtx};
				m_handle->m_executor = nullptr;
				m_parent->m_multi.remove_handle(*m_handle);
				cb(CURLE_ABORTED_BY_CALLBACK);
			});
		}
	}

	executor::exec_awaiter executor::exec(handle& hdl, std::stop_token st) {
		if (hdl.is_connect_only()) throw std::logic_error("unsupported");
		return exec_awaiter{this, &hdl, std::move(st)};
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

	void executor::wakeup() {
		if (m_thread.get_id() != std::this_thread::get_id()) m_multi.wakeup();
	}

	executor& executor::get_default() {
		static executor instance{};
		return instance;
	}
} // namespace asyncpp::curl
