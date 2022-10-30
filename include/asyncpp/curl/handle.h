#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>

namespace asyncpp::curl {
	class multi;
	class executor;
	class slist;
	/**
	 * \brief Wrapper around a libcurl handle.
	 * 
	 * This is a fat wrapper, it contains additional members to allow usage of C++ functions and handle raii.
	 */
	class handle {
		mutable std::recursive_mutex m_mtx;
		void* m_instance;
		multi* m_multi;
		executor* m_executor;
		std::function<void(int result)> m_done_callback{};
		std::function<size_t(char* buffer, size_t size)> m_header_callback{};
		std::function<size_t(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)> m_progress_callback{};
		std::function<size_t(char* buffer, size_t size)> m_read_callback{};
		std::function<size_t(char* buffer, size_t size)> m_write_callback{};
		std::map<int, slist> m_owned_slists;
		uint32_t m_flags;

		friend class multi;
		friend class executor;

	public:
		/** \brief Construct a new libcurl handle */
		handle();
		/** \brief Destructor */
		~handle() noexcept;
		handle(const handle&) = delete;
		handle& operator=(const handle&) = delete;
		handle(handle&&) = delete;
		handle& operator=(handle&&) = delete;

		/**
		 * \brief Get the raw CURL handle. Use with caution.
		 */
		void* raw() const noexcept { return m_instance; }

		/**
		 * \brief Set an option of type long
		 * \param opt Curl option to set
		 * \param val Value to use for option
		 */
		void set_option_long(int opt, long val);
		/**
		 * \brief Set an option of type offset
		 * \param opt Curl option to set
		 * \param val Value to use for option
		 */
		void set_option_offset(int opt, long val);
		/**
		 * \brief Set an option of type pointer (callback fn, void*)
		 * \param opt Curl option to set
		 * \param val Value to use for option
		 */
		void set_option_ptr(int opt, const void* ptr);
		/**
		 * \brief Set an option of type string
		 * \param opt Curl option to set
		 * \param val Value to use for option
		 */
		void set_option_string(int opt, const char* str);
		/**
		 * \brief Set an option of type blob
		 * \param opt Curl option to set
		 * \param data The pointer to the data used to set
		 * \param data_size The size of the data pointed to by data in bytes
		 * \param copy Copy the data into curl. If false the memory pointed to by data needs to stay valid until the handle is destroyed or this option is set again.
		 */
		void set_option_blob(int opt, void* data, size_t data_size, bool copy = true);
		/**
		 * \brief Set an option of type bool/long(0,1)
		 * \param opt Curl option to set
		 * \param val Value to use for option
		 */
		void set_option_bool(int opt, bool on);
		/**
		 * \brief Set an option of type slist
		 * \param opt Curl option to set
		 * \param val Value to use for option
		 * \note The handle class copies the slist and keeps it alive until this option is changed.
		 */
		void set_option_slist(int opt, slist list);

		/**
		 * \brief Set the handle url
		 * \param url The handle url
		 */
		void set_url(const char* url);
		/**
		 * \brief Set the handle url
		 * \param url The handle url
		 */
		void set_url(const std::string& url);
		/**
		 * \brief Set if curl should follow locations
		 * \param on true to follow
		 */
		void set_follow_location(bool on);
		/**
		 * \brief Enable/Disable verbose output
		 * \param on true to enable
		 */
		void set_verbose(bool on);
		/**
		 * \brief Set the request headers
		 * \param list slist containing all request headers
		 */
		void set_headers(slist list);

		/**
		 * \brief Set a function to be called by curl if data is received
		 * \param cb Callback to be called
		 * \note ptr points to a block os size bytes without zero termination.
		 * \note The function can return CURL_WRITEFUNC_PAUSE to pause the transfer.
		 * \note If the value returned differs from size the transfer is aborted with CURLE_WRITE_ERROR.
		 * \note If the handle uses CURLOPT_CONNECT_ONLY and passed to a executor instance this function gets invoked with nullptr when the socket is readable.
		 */
		void set_writefunction(std::function<size_t(char* ptr, size_t size)> cb);
		/**
		 * \brief Set a stream to write received data to. This removes any set writefunction.
		 * \param stream Output stream used for writing.
		 * \note The stream need to stay valid until the transfer is done
		 */
		void set_writestream(std::ostream& stream);
		/**
		 * \brief Set a string variable to store receivd data into. This removes any set writefunction.
		 * \param str String variable used to store received data.
		 * \note The string need to stay valid until the transfer is done
		 */
		void set_writestring(std::string& str);

		/**
		 * \brief Set a function to be called by curl if data is sent to the remote
		 * \param cb Callback to be called
		 * \note ptr points to a block os size bytes without zero termination.
		 * \note The function can return CURL_READFUNC_PAUSE to pause the transfer or CURL_READFUNC_ABORT to abort.
		 * \note The function shall return the number of bytes stored in ptr or 0 to signal EOF.
		 * \note If the handle uses CURLOPT_CONNECT_ONLY and passed to a executor instance this function gets invoked with nullptr when the socket is writable.
		 */
		void set_readfunction(std::function<size_t(char* ptr, size_t size)> cb);
		/**
		 * \brief Set a stream to read transmitted data from. This removes any set readfunction.
		 * \param stream Input stream used for reading.
		 * \note The stream need to stay valid until the transfer is done
		 */
		void set_readstream(std::istream& stream);
		/**
		 * \brief Set a string variable to read transmitted data from. This removes any set readfunction.
		 * \param str String variable used to transmitt.
		 * \note The string need to stay valid until the transfer is done
		 */
		void set_readstring(const std::string& str);

		/**
		 * \brief Set a function to be called with progress information
		 * \param cb Callback to be called
		 * \note The function should return 0. Returning any non-zero value aborts the transfer.
		 */
		void set_progressfunction(std::function<int(int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow)> cb);
		/**
		 * \brief Set a function to be called by curl if a header is received
		 * \param cb Callback to be called
		 * \note buffer points to a block os size bytes without zero termination.
		 * \note If the value returned differs from size the transfer is aborted with CURLE_WRITE_ERROR.
		 */
		void set_headerfunction(std::function<size_t(char* buffer, size_t size)> cb);
		/**
		 * \brief Set a slist instance to store received headers into.
		 * \param list The slist to store headers into.
		 * \note The library detects line breaks and emits full lines to the slist.
		 * \note The slist needs to stay valid for the entire duration of the transfer.
		 */
		void set_headerfunction_slist(slist& list);
		/**
		 * \brief Set a function to invoke once the transfer is completed
		 * \param cb Callback being invoked after completion with the result code.
		 * \note This callback is mostly usefull with the multi interface, but it is also called in synchronous operation.
		 */
		void set_donefunction(std::function<void(int result)> cb);

		/**
		 * \brief Perform the transfer synchronously. This blocks until the operation finished.
		 * \note Use the multi interface if you need asynchronous operation.
		 */
		void perform();
		/**
		 * \brief Reset the handle to the same state a freshly constructed handle has.
		 * \note If the handle has an ongoing multi transfer, it is stopped and removed from the multi handle.
		 */
		void reset();

		/**
		 * \brief Perform connection upkeep work (keep alives) if supported.
		 */
		void upkeep();
		/**
		 * \brief Receive data on a CURLOPT_CONNECT_ONLY connection
		 * \param buffer Buffer to read data into
		 * \param buflen Size of the buffer to read into
		 * \return ssize_t Number of bytes read, or
		 *			- -1 if no data is available (EAGAIN)
		 *			- 0 if the connection was closed
		 */
		std::ptrdiff_t recv(void* buffer, size_t buflen);
		/**
		 * \brief Send data on a CURLOPT_CONNECT_ONLY connection
		 * \param buffer Buffer to send
		 * \param buflen Size of the buffer to send
		 * \return ssize_t Number of bytes sent, or
		 *			- -1 if no the connection buffer is full (EAGAIN)
		 *			- 0 if the connection was closed
		 */
		std::ptrdiff_t send(const void* buffer, size_t buflen);
		/**
		 * \brief Check if this handle has the CURLOPT_CONNECT_ONLY option set.
		 *	If this option is set the handle wont be automatically removed
		 *	from a executor when the done callback is executed.
		 */
		bool is_connect_only() const noexcept;
		/**
		 * \brief Check if this handle has the CURLOPT_VERBOSE option set.
		 */
		bool is_verbose() const noexcept;

		/**
		 * \brief Pause the transfer.
		 * \param dirs Directions to pause (CURLPAUSE_RECV, CURLPAUSE_SEND or CURLPAUSE_ALL)
		 * \note Unlike the regular curl_easy_pause function this keeps track of previous pauses and can not be used for unpausing.
		 */
		void pause(int dirs);
		/**
		 * \brief Unpause the transfer.
		 * \param dirs Directions to unpause (CURLPAUSE_RECV, CURLPAUSE_SEND or CURLPAUSE_ALL)
		 * \note See pause() for details.
		 */
		void unpause(int dirs);
		/**
		 * \brief Check if the transfer is paused.
		 * \param dir CURLPAUSE_RECV or CURLPAUSE_SEND
		 * \return true if the transfer is paused in this direction 
		 */
		bool is_paused(int dir);

		/**
		 * \brief Get a info element of type long
		 * \param info The id of the info to get
		 * \return The requested info
		 */
		long get_info_long(int info) const;
		/**
		 * \brief Get a info element of type socket
		 * \param info The id of the info to get
		 * \return The requested socket
		 */
		uint64_t get_info_socket(int info) const;
		/**
		 * \brief Get a info element of type double
		 * \param info The id of the info to get
		 * \return The requested info
		 */
		double get_info_double(int info) const;
		/**
		 * \brief Get a info element of type string
		 * \param info The id of the info to get
		 * \return The requested info (do not free the returned value)
		 */
		const char* get_info_string(int info) const;
		/**
		 * \brief Get a info element of type slist
		 * \param info The id of the info to get
		 * \return The requested info
		 */
		slist get_info_slist(int info) const;

		/**
		 * \brief Get the http code of the last transfer.
		 * \return The http status code
		 */
		long get_response_code() const;

		/**
		 * \brief Get a pointer to the handle class from a raw curl structure.
		 * \note Make sure the curl handle is actually part of a handle class, otherwise this will result in memory corruption.
		 * \param curl The raw handle previously retrieved using raw().
		 * \return handle* A pointer to the wrapping handle.
		 */
		static handle* get_handle_from_raw(void* curl);
	};
} // namespace asyncpp::curl
