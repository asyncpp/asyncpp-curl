#pragma once
#include <string>
#include <functional>

namespace asyncpp::curl {
    class multi;
    class executor;
    class slist;
    class handle {
        void* m_instance;
        multi* m_multi;
        std::function<void(int result)> m_done_callback{};
        std::function<size_t(char *buffer, size_t size)> m_header_callback{};
        std::function<size_t(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)> m_progress_callback{};
        std::function<size_t(char *buffer, size_t size)> m_read_callback{};
        std::function<size_t(char *buffer, size_t size)> m_write_callback{};
        int m_pause_state;
        friend class multi;
        friend class executor;
    public:
        handle();
        handle(const handle&) = delete;
        handle& operator=(const handle&) = delete;
        handle(handle&&) = delete;
        handle& operator=(handle&&) = delete;
        ~handle() noexcept;

        void* raw() const noexcept { return m_instance; }

        void set_option_long(int opt, long val);
        void set_option_ptr(int opt, const void* str);
        void set_option_bool(int opt, bool on);
        void set_option_slist(int opt, const slist& list);

        void set_url(const char* url);
        void set_url(const std::string& url);
        void set_follow_location(bool on);
        void set_verbose(bool on);
        void set_headers(const slist& list);

        void set_writefunction(std::function<size_t(char *ptr, size_t size)> cb);
        void set_writestream(std::ostream& stream);
        void set_writestring(std::string& str);

        void set_readfunction(std::function<size_t(char *ptr, size_t size)> cb);
        void set_readstream(std::istream& stream);
        void set_readstring(const std::string& str);

        void set_progressfunction(std::function<int(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)> cb);
        void set_headerfunction(std::function<size_t(char *buffer, size_t size)> cb);
        void set_headerfunction_slist(slist& list);
        void set_donefunction(std::function<void(int result)> cb);

        void perform();
        void reset();

        void pause(int dirs);
        void unpause(int dirs);
        bool is_paused(int dir);

        long get_info_long(int info) const;
        double get_info_double(int info) const;
        const char* get_info_string(int info) const;

        long get_response_code() const;

        static handle* get_handle_from_raw(void* curl);

    };
}