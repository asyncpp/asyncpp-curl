#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/executor.h>
#include <asyncpp/curl/handle.h>
#include <asyncpp/curl/multi.h>
#include <asyncpp/curl/slist.h>
#include <cstring>
#include <curl/curl.h>
#include <istream>
#include <mutex>
#include <ostream>
#include <stdexcept>

namespace asyncpp::curl {
	constexpr static uint32_t FLAG_is_connect_only = 1 << 1;
	static_assert((FLAG_is_connect_only & CURLPAUSE_ALL) == 0, "Overlap between custom flags and CURLPAUSE_ALL");
	constexpr static uint32_t FLAG_is_verbose = 1 << 3;
	static_assert((FLAG_is_verbose & CURLPAUSE_ALL) == 0, "Overlap between custom flags and CURLPAUSE_ALL");

	handle::handle() : m_instance{nullptr}, m_multi{nullptr}, m_executor{nullptr}, m_flags{0} {
		m_instance = curl_easy_init();
		if (!m_instance) throw std::runtime_error("failed to create curl handle");
		set_option_ptr(CURLOPT_PRIVATE, this);
		// We set CURLOPT_NOSIGNAL to avoid errors in MT, this is fine in most cases.
		set_option_bool(CURLOPT_NOSIGNAL, true);
	}

	handle::~handle() noexcept {
		if (m_executor) m_executor->remove_handle(*this);
		if (m_multi) m_multi->remove_handle(*this);
		if (m_instance) curl_easy_cleanup(m_instance);
	}

	void handle::set_option_long(int opt, long val) {
		// TODO: Evaluate curl_easy_option_by_id for checking
		if (int base = (opt / 10000) * 10000; base != CURLOPTTYPE_LONG) throw std::invalid_argument("invalid option supplied to set_option_long");
		std::scoped_lock lck{m_mtx};
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), val);
		if (res != CURLE_OK) throw exception{res};
		if (opt == CURLOPT_CONNECT_ONLY)
			m_flags = val ? (m_flags | FLAG_is_connect_only) : (m_flags & ~FLAG_is_connect_only);
		else if (opt == CURLOPT_VERBOSE)
			m_flags = val ? (m_flags | FLAG_is_verbose) : (m_flags & ~FLAG_is_verbose);
	}

	void handle::set_option_offset(int opt, long val) {
		// TODO: Evaluate curl_easy_option_by_id for checking
		if (int base = (opt / 10000) * 10000; base != CURLOPTTYPE_OFF_T) throw std::invalid_argument("invalid option supplied to set_option_long");
		std::scoped_lock lck{m_mtx};
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), static_cast<curl_off_t>(val));
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_option_ptr(int opt, const void* ptr) {
		// TODO: Evaluate curl_easy_option_by_id for checking
		if (int base = (opt / 10000) * 10000; base != CURLOPTTYPE_OBJECTPOINT && base != CURLOPTTYPE_FUNCTIONPOINT)
			throw std::invalid_argument("invalid option supplied to set_option_ptr");
		std::scoped_lock lck{m_mtx};
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), ptr);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_option_string(int opt, const char* str) {
		// TODO: Evaluate curl_easy_option_by_id for checking
		if (int base = (opt / 10000) * 10000; base != CURLOPTTYPE_STRINGPOINT) throw std::invalid_argument("invalid option supplied to set_option_ptr");
		std::scoped_lock lck{m_mtx};
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), str);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_option_blob(int opt, void* data, size_t data_size, bool copy) {
		// TODO: Evaluate curl_easy_option_by_id for checking
		if (int base = (opt / 10000) * 10000; base != CURLOPTTYPE_BLOB) throw std::invalid_argument("invalid option supplied to set_option_blob");
		curl_blob b{.data = data, .len = data_size, .flags = static_cast<unsigned int>(copy ? CURL_BLOB_COPY : CURL_BLOB_NOCOPY)};
		std::scoped_lock lck{m_mtx};
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), &b);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::set_option_bool(int opt, bool on) { set_option_long(opt, static_cast<long>(on ? 1 : 0)); }

	void handle::set_option_slist(int opt, slist list) {
		// TODO: Evaluate curl_easy_option_by_id for checking
		if (int base = (opt / 10000) * 10000; base != CURLOPTTYPE_SLISTPOINT) throw std::invalid_argument("invalid option supplied to set_option_ptr");
		std::scoped_lock lck{m_mtx};
		auto res = curl_easy_setopt(m_instance, static_cast<CURLoption>(opt), list.m_first_node);
		if (res != CURLE_OK) throw exception{res};
		m_owned_slists[opt] = std::move(list);
	}

	void handle::set_url(const char* url) { set_option_ptr(CURLOPT_URL, url); }

	void handle::set_url(const std::string& url) { return set_url(url.c_str()); }

	void handle::set_follow_location(bool on) { set_option_bool(CURLOPT_FOLLOWLOCATION, on); }

	void handle::set_verbose(bool on) { set_option_bool(CURLOPT_VERBOSE, on); }

	void handle::set_headers(slist list) { set_option_slist(CURLOPT_HTTPHEADER, std::move(list)); }

	void handle::set_writefunction(std::function<size_t(char* ptr, size_t size)> cb) {
		constexpr curl_write_callback real_cb = [](char* buffer, size_t size, size_t nmemb, void* udata) -> size_t {
			auto res = static_cast<handle*>(udata)->m_write_callback(buffer, size * nmemb);
			if (res == CURL_WRITEFUNC_PAUSE) static_cast<handle*>(udata)->m_flags |= CURLPAUSE_RECV;
			return res;
		};

		std::scoped_lock lck{m_mtx};
		m_write_callback = cb;
		auto res = curl_easy_setopt(m_instance, CURLOPT_WRITEFUNCTION, real_cb);
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
		curl_read_callback real_cb = [](char* buffer, size_t size, size_t nmemb, void* udata) -> size_t {
			auto res = static_cast<handle*>(udata)->m_read_callback(buffer, size * nmemb);
			if (res == CURL_READFUNC_PAUSE) static_cast<handle*>(udata)->m_flags |= CURLPAUSE_SEND;
			return res;
		};

		std::scoped_lock lck{m_mtx};
		m_read_callback = cb;
		auto res = curl_easy_setopt(m_instance, CURLOPT_READFUNCTION, real_cb);
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
		constexpr curl_xferinfo_callback real_cb = [](void* udata, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) -> int {
			return static_cast<handle*>(udata)->m_progress_callback(dltotal, dlnow, ultotal, ulnow);
		};

		std::scoped_lock lck{m_mtx};
		m_progress_callback = cb;
		auto res = curl_easy_setopt(m_instance, CURLOPT_XFERINFOFUNCTION, real_cb);
		if (res != CURLE_OK) throw exception{res};
		set_option_ptr(CURLOPT_XFERINFODATA, this);
		set_option_bool(CURLOPT_NOPROGRESS, false);
	}

	void handle::set_headerfunction(std::function<size_t(char* buffer, size_t size)> cb) {
		constexpr curl_read_callback real_cb = [](char* buffer, size_t size, size_t nmemb, void* udata) -> size_t {
			return static_cast<handle*>(udata)->m_header_callback(buffer, size * nmemb);
		};

		std::scoped_lock lck{m_mtx};
		m_header_callback = cb;
		auto res = curl_easy_setopt(m_instance, CURLOPT_HEADERFUNCTION, real_cb);
		if (res != CURLE_OK) throw exception{res};
		set_option_ptr(CURLOPT_HEADERDATA, this);
	}

	void handle::set_headerfunction_slist(slist& list) {
		set_headerfunction([&list](char* buffer, size_t size) -> size_t {
			if (size == 0 || size == 1) return size;
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

	void handle::set_donefunction(std::function<void(int result)> cb) {
		std::scoped_lock lck{m_mtx};
		m_done_callback = cb;
	}

	void handle::perform() {
		std::unique_lock lck{m_mtx};
		if (m_multi) throw std::logic_error("perform called on handle in multi");
		auto res = curl_easy_perform(m_instance);
		auto cb = m_done_callback;
		lck.unlock();
		if (cb) cb(res);
		if (res != CURLE_OK) throw exception{res};
	}

	void handle::reset() {
		if (m_executor) m_executor->remove_handle(*this);
		if (m_multi) m_multi->remove_handle(*this);
		std::scoped_lock lck{m_mtx};
		curl_easy_reset(m_instance);
		set_option_ptr(CURLOPT_PRIVATE, this);
		m_done_callback = {};
		m_header_callback = {};
		m_progress_callback = {};
		m_read_callback = {};
		m_write_callback = {};
		m_flags = 0;
		m_owned_slists.clear();
	}

	void handle::upkeep() {
#if LIBCURL_VERSION_NUM >= 0x073E00
		std::scoped_lock lck{m_mtx};
		auto res = curl_easy_upkeep(m_instance);
		if (res != CURLE_OK) throw exception{res};
#endif
	}

	ssize_t handle::recv(void* buffer, size_t buflen) {
		std::scoped_lock lck{m_mtx};
		size_t read{};
		auto res = curl_easy_recv(m_instance, buffer, buflen, &read);
		if (res == CURLE_AGAIN) return -1;
		if (res != CURLE_OK) throw exception{res};
		return read;
	}

	ssize_t handle::send(const void* buffer, size_t buflen) {
		std::scoped_lock lck{m_mtx};
		size_t sent{};
		auto res = curl_easy_send(m_instance, buffer, buflen, &sent);
		if (res == CURLE_AGAIN) return -1;
		if (res != CURLE_OK) throw exception{res};
		return sent;
	}

	bool handle::is_connect_only() const noexcept {
		std::scoped_lock lck{m_mtx};
		return m_flags & FLAG_is_connect_only;
	}

	bool handle::is_verbose() const noexcept {
		std::scoped_lock lck{m_mtx};
		return m_flags & FLAG_is_verbose;
	}

	void handle::pause(int dirs) {
		std::scoped_lock lck{m_mtx};
		auto old = m_flags;
		m_flags |= (dirs & CURLPAUSE_ALL);
		if (m_flags == old) return;
		curl_easy_pause(m_instance, m_flags & CURLPAUSE_ALL);
	}

	void handle::unpause(int dirs) {
		std::unique_lock lck{m_mtx};
		auto old = m_flags;
		m_flags &= ~(dirs & CURLPAUSE_ALL);
		if (m_flags == old) return;
		curl_easy_pause(m_instance, m_flags & CURLPAUSE_ALL);
		// Wake the executor/multi if theres any to make sure it gets polled soon
		if (m_executor)
			m_executor->wakeup();
		else if (m_multi)
			m_multi->wakeup();
	}

	bool handle::is_paused(int dir) {
		std::scoped_lock lck{m_mtx};
		return (m_flags & dir) != 0;
	}

	long handle::get_info_long(int info) const {
		if ((info & CURLINFO_TYPEMASK) != CURLINFO_LONG) throw std::invalid_argument("invalid info supplied to get_info_long");
		std::scoped_lock lck{m_mtx};
		long p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		return p;
	}

	uint64_t handle::get_info_socket(int info) const {
		if ((info & CURLINFO_TYPEMASK) != CURLINFO_SOCKET) throw std::invalid_argument("invalid info supplied to get_info_socket");
		std::scoped_lock lck{m_mtx};
		curl_socket_t p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		if (p == CURL_SOCKET_BAD) return std::numeric_limits<uint64_t>::max();
		return p;
	}

	double handle::get_info_double(int info) const {
		if ((info & CURLINFO_TYPEMASK) != CURLINFO_DOUBLE) throw std::invalid_argument("invalid info supplied to get_info_double");
		std::scoped_lock lck{m_mtx};
		double p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		return p;
	}

	const char* handle::get_info_string(int info) const {
		if ((info & CURLINFO_TYPEMASK) != CURLINFO_STRING) throw std::invalid_argument("invalid info supplied to get_info_string");
		std::scoped_lock lck{m_mtx};
		char* p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		return p;
	}

	slist handle::get_info_slist(int info) const {
		if ((info & CURLINFO_TYPEMASK) != CURLINFO_SLIST) throw std::invalid_argument("invalid info supplied to get_info_slist");
		std::scoped_lock lck{m_mtx};
		struct curl_slist* p;
		auto res = curl_easy_getinfo(m_instance, static_cast<CURLINFO>(info), &p);
		if (res != CURLE_OK) throw exception{res};
		return slist{p, slist::ownership_take};
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
