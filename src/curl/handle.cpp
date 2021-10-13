#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/handle.h>
#include <asyncpp/curl/multi.h>
#include <asyncpp/curl/slist.h>
#include <cstring>
#include <curl/curl.h>
#include <istream>
#include <ostream>

namespace asyncpp::curl {
	handle::handle() : m_instance{nullptr}, m_multi{nullptr}, m_pause_state{0} {
		// TODO: Needs a mutex
		m_instance = curl_easy_init();
		if (!m_instance) throw std::runtime_error("failed to create curl handle");
		set_option_ptr(CURLOPT_PRIVATE, this);
	}

	//handle::handle(const handle& other)
	//    : m_instance{curl_easy_duphandle(other.m_instance)}, m_multi{nullptr}
	//{
	//    if(!m_instance) throw std::runtime_error("failed to copy handle");
	//    set_option_ptr(CURLOPT_PRIVATE, this);
	//}

	//handle& handle::operator=(const handle& other)
	//{
	//    if(m_multi) m_multi->remove_handle(*this);
	//    auto hdl = curl_easy_duphandle(other.m_instance);
	//    if(!hdl) throw std::runtime_error("failed to copy handle");
	//    if(m_instance)
	//        curl_easy_cleanup(m_instance);
	//    m_instance = hdl;
	//    set_option_ptr(CURLOPT_PRIVATE, this);
	//    return *this;
	//}

	handle::~handle() noexcept {
		if (m_multi) m_multi->remove_handle(*this);
		if (m_instance) curl_easy_cleanup(m_instance);
	}

	void handle::set_option_long(int opt, long val) {
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), val);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_option_ptr(int opt, const void* str) {
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), str);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_option_bool(int opt, bool on) {
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), static_cast<long>(on ? 1 : 0));
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_option_slist(int opt, const slist& list) {
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), list.m_first_node);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_url(const char* url) { set_option_ptr(CURLOPT_URL, url); }

	void handle::set_url(const std::string& url) { return set_url(url.c_str()); }

	void handle::set_follow_location(bool on) { set_option_bool(CURLOPT_FOLLOWLOCATION, on); }

	void handle::set_verbose(bool on) { set_option_bool(CURLOPT_VERBOSE, on); }

	void handle::set_headers(const slist& list) { set_option_slist(CURLOPT_HTTPHEADER, list); }

	void handle::set_writefunction(std::function<size_t(char* ptr, size_t size)> cb) {
		m_write_callback = cb;
		auto res = curl_easy_setopt(m_instance, CURLOPT_WRITEFUNCTION,
									static_cast<curl_read_callback>([](char* buffer, size_t size, size_t nmemb, void* udata) -> size_t {
										auto res = static_cast<handle*>(udata)->m_write_callback(buffer, size * nmemb);
										if (res == CURL_WRITEFUNC_PAUSE) static_cast<handle*>(udata)->m_pause_state |= CURLPAUSE_RECV;
										return res;
									}));
		if (res != CURLE_OK) throw exception{res};
		set_option_ptr(CURLOPT_WRITEDATA, this);
	}

	void handle::set_writestream(std::ostream& stream) {
		set_writefunction([&stream](char* ptr, size_t size) -> size_t {
			if (size == 0) return 0;
			stream.write(ptr, size);
			return size;
		});
	}

	void handle::set_writestring(std::string& str) {
		set_writefunction([&str](char* ptr, size_t size) -> size_t {
			if (size == 0) return 0;
			str.append(ptr, size);
			return size;
		});
	}

	void handle::set_readfunction(std::function<size_t(char* ptr, size_t size)> cb) {
		m_read_callback = cb;
		auto res = curl_easy_setopt(m_instance, CURLOPT_READFUNCTION,
									static_cast<curl_read_callback>([](char* buffer, size_t size, size_t nmemb, void* udata) -> size_t {
										auto res = static_cast<handle*>(udata)->m_read_callback(buffer, size * nmemb);
										if (res == CURL_READFUNC_PAUSE) static_cast<handle*>(udata)->m_pause_state |= CURLPAUSE_SEND;
										return res;
									}));
		if (res != CURLE_OK) throw exception{res};
		set_option_ptr(CURLOPT_READDATA, this);
	}

	void handle::set_readstream(std::istream& stream) {
		set_readfunction([&stream](char* ptr, size_t size) -> size_t {
			if (size == 0 || stream.eof()) return 0;
			if (stream.fail()) return CURLE_ABORTED_BY_CALLBACK;
			return stream.read(ptr, size).gcount();
		});
	}

	void handle::set_readstring(const std::string& str) {
		size_t pos = 0;
		set_readfunction([&str, pos](char* ptr, size_t size) mutable -> size_t {
			if (size == 0 || str.size() == pos) return 0;
			auto read = std::min(size, str.size() - pos);
			memcpy(ptr, str.data() + pos, read);
			pos += read;
			return read;
		});
	}

	void handle::set_progressfunction(std::function<int(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)> cb) {
		m_progress_callback = cb;
		auto res =
			curl_easy_setopt(m_instance, CURLOPT_XFERINFOFUNCTION, [](void* udata, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) -> int {
				return static_cast<handle*>(udata)->m_progress_callback(dltotal, dlnow, ultotal, ulnow);
			});
		if (res != CURLE_OK) throw exception{res};
		set_option_ptr(CURLOPT_XFERINFODATA, this);
		set_option_bool(CURLOPT_NOPROGRESS, false);
	}

	void handle::set_headerfunction(std::function<size_t(char* buffer, size_t size)> cb) {
		m_header_callback = cb;
		auto res = curl_easy_setopt(m_instance, CURLOPT_HEADERFUNCTION,
									static_cast<curl_read_callback>([](char* buffer, size_t size, size_t nmemb, void* udata) -> size_t {
										return static_cast<handle*>(udata)->m_header_callback(buffer, size * nmemb);
									}));
		if (res != CURLE_OK) throw exception{res};
		set_option_ptr(CURLOPT_HEADERDATA, this);
	}

	void handle::set_headerfunction_slist(slist& list) {
		set_headerfunction([&list](char* buffer, size_t size) -> size_t {
			if (size == 0) return 0;
			// A complete HTTP header [...] includes the final line terminator.
			// We temporarily replace it with a null character for append.
			// This saves us from having to copy the string.
			auto c = buffer[size - 1];
			buffer[size - 1] = '\0';
			try {
				list.append(buffer);
			} catch (...) {
				buffer[size - 1] = c;
				return 0; // Write error
			}
			buffer[size - 1] = c;
			return size;
		});
	}

	void handle::set_donefunction(std::function<void(int result)> cb) { m_done_callback = cb; }

	void handle::perform() {
		if (m_multi) throw std::logic_error("performed called on handle in multi");
		auto res = curl_easy_perform(m_instance);
		if (m_done_callback) m_done_callback(res);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::reset() {
		if (m_multi) m_multi->remove_handle(*this);
		m_done_callback = {};
		curl_easy_reset(m_instance);
		set_option_ptr(CURLOPT_PRIVATE, this);
	}

	void handle::pause(int dirs) {
		auto old = m_pause_state;
		m_pause_state |= (dirs & CURLPAUSE_ALL);
		if (m_pause_state == old) return;
		curl_easy_pause(m_instance, m_pause_state);
	}

	void handle::unpause(int dirs) {
		auto old = m_pause_state;
		m_pause_state &= ~(dirs & CURLPAUSE_ALL);
		if (m_pause_state == old) return;
		curl_easy_pause(m_instance, m_pause_state);
	}

	bool handle::is_paused(int dir) { return (m_pause_state & dir) != 0; }

	long handle::get_info_long(int info) const {
		long p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		return p;
	}

	double handle::get_info_double(int info) const {
		double p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		return p;
	}

	const char* handle::get_info_string(int info) const {
		char* p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		return p;
	}

	long handle::get_response_code() const { return get_info_long(CURLINFO_RESPONSE_CODE); }

	handle* handle::get_handle_from_raw(void* curl) {
		if (!curl) return nullptr;
		char* ptr = nullptr;
		auto res = curl_easy_getinfo(curl, CURLINFO_PRIVATE, &ptr);
		if (res != CURLE_OK) throw exception{res};
		return reinterpret_cast<handle*>(ptr);
	}
} // namespace asyncpp::curl