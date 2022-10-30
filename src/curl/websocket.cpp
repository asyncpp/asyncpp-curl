#include <asyncpp/curl/base64.h>
#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/executor.h>
#include <asyncpp/curl/sha1.h>
#include <asyncpp/curl/tcp_client.h>
#include <asyncpp/curl/uri.h>
#include <asyncpp/curl/websocket.h>

#include <asyncpp/event.h>
#include <asyncpp/generator.h>
#include <asyncpp/launch.h>
#include <asyncpp/task.h>
#include <asyncpp/threadsafe_queue.h>

#include <cstring>
#include <curl/curl.h>
#include <iostream>
#include <random>
#include <string_view>

namespace {
	static const std::string ws_magic_string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	using namespace asyncpp;
	typedef curl::websocket::opcode opcode;

	task<bool> read_until(curl::tcp_client& client, std::string_view delim, std::string& result, std::string& extra) {
		static constexpr size_t chunk_size = 64 * 1024;
		result.resize(chunk_size);
		size_t old_size = 0;
		auto size = co_await client.recv(result.data() + old_size, result.size() - old_size);
		while (size > 0) {
			result.resize(old_size + size);
			auto pos = result.find(delim);
			if (pos != std::string::npos) {
				extra = result.substr(pos + delim.size());
				result.resize(pos);
				co_return true;
			}
			old_size += size;
			result.resize(old_size + chunk_size);
			size = co_await client.recv(result.data() + old_size, result.size() - old_size);
		}
		co_return false;
	}

	static thread_local std::mt19937 g_random(std::random_device{}());

	template<typename T>
	T get_be(const void* ptr) {
		T result;
		uint8_t* ptr_out = reinterpret_cast<uint8_t*>(&result);
		const uint8_t* ptr_in = static_cast<const uint8_t*>(ptr);
		if constexpr (std::endian::native == std::endian::little)
			std::reverse_copy(ptr_in, ptr_in + sizeof(T), ptr_out);
		else
			std::copy(ptr_in, ptr_in + sizeof(T), ptr_out);
		return result;
	}
	template<typename T>
	void set_be(void* ptr, T val) {
		uint8_t* ptr_out = static_cast<uint8_t*>(ptr);
		const uint8_t* ptr_in = reinterpret_cast<const uint8_t*>(&val);
		if constexpr (std::endian::native == std::endian::little)
			std::reverse_copy(ptr_in, ptr_in + sizeof(T), ptr_out);
		else
			std::copy(ptr_in, ptr_in + sizeof(T), ptr_out);
	}

	curl::websocket::opcode clean_op(curl::websocket::opcode op) { return static_cast<curl::websocket::opcode>(static_cast<uint8_t>(op) & 0x0f); }
} // namespace

namespace asyncpp::curl {
	namespace detail {

		struct websocket_state {
			websocket_state(executor& e) noexcept : client(e) {}

			mutable thread_safe_refcount m_refcount{0};
			std::recursive_mutex mtx;
			tcp_client client;

			// Handshake info
			uri last_url;
			std::string handshake_nonce;
			websocket::header_map request_headers;
			websocket::header_map response_headers;

			// Callbacks
			std::function<void(int)> on_open;
			std::function<void(uint16_t, std::string_view)> on_close;
			std::function<void(websocket::buffer, bool)> on_message;
			std::function<void(websocket::buffer)> on_ping;
			std::function<void(websocket::buffer)> on_pong;

			// Async management
			async_launch_scope async_scope{};

			opcode fragment_code = opcode::continuation;
			std::string fragment_payload;
			size_t fragment_validate_offset = 0;

			enum class cs { init, connect, handshake, open, client_close, server_close, closed };
			std::atomic<cs> con_state{cs::init};

			std::atomic<utf8_validator::mode> utf8_mode{utf8_validator::mode::pedantic};

			// Parser state
			size_t parser_wanted_size{2};
			std::string parser_data;

			// Transmit side
			std::atomic<bool> send_should_exit{false};
			single_consumer_event send_event;
			threadsafe_queue<std::pair<std::string, std::function<void(bool)>>> send_queue;

			// Connection state handling
			cs state_transition(cs newstate);
			void raw_on_connect(int code);
			void raw_on_disconnect();
			tcp_client::callback_result raw_on_data_available(bool cancelled);

			// Frame handling
			void handle_frame(opcode code, std::string payload);
			void handle_close_frame(opcode code, std::string payload);
			void handle_ping_frame(opcode code, std::string payload);
			void handle_pong_frame(opcode code, std::string payload);
			void handle_text_frame(opcode code, std::string payload);
			void handle_binary_frame(opcode code, std::string payload);
			void handle_continuation_frame(opcode code, std::string payload);

			// Api
			void connect(const uri& url);
			void close(uint16_t code, std::string_view reason);
			void send_frame(opcode op, const websocket::buffer buf, std::function<void(bool)> cb);
		};

		void refcounted_add_ref(const websocket_state* ptr) noexcept {
			assert(ptr);
			ptr->m_refcount.fetch_increment();
		}

		void refcounted_remove_ref(const websocket_state* ptr) noexcept {
			if (ptr == nullptr) return;
			auto cnt = ptr->m_refcount.fetch_decrement();
			if (cnt == 1) delete ptr;
		}
	} // namespace detail

	websocket::websocket() : websocket(executor::get_default()) {}

	websocket::websocket(executor& e) : m_state(make_ref<detail::websocket_state>(e)) {
		m_state->request_headers.emplace("User-Agent", std::string("asyncpp-curl, libcurl ") + curl_version());
		m_state->request_headers.emplace("Connection", "Upgrade");
		m_state->request_headers.emplace("Upgrade", "websocket");
		m_state->request_headers.emplace("Sec-WebSocket-Version", "13");
		std::uniform_int_distribution<short> dist((std::numeric_limits<char>::min)(), (std::numeric_limits<char>::max)());
		std::string nonce;
		nonce.resize(16);
		std::generate(nonce.begin(), nonce.end(), [dist]() mutable -> char { return dist(g_random); });
		nonce = base64::encode(nonce);
		m_state->request_headers.emplace("Sec-WebSocket-Key", nonce);
		m_state->handshake_nonce = base64::encode(sha1::hash(nonce + ws_magic_string));
	}

	websocket::~websocket() {
		launch([](ref<detail::websocket_state> state) -> task<void> {
			if (state->client.is_connected() && state->con_state != detail::websocket_state::cs::closed) { state->close(1002, "Going away"); }
			state->send_should_exit = true;
			state->send_event.set();
			co_await state->async_scope.join();
		}(m_state));
	}

	void websocket::connect(const uri& url) { m_state->connect(url); }

	void websocket::close(uint16_t code, std::string_view reason) {
		if (utf8_validator{}(reason, m_state->utf8_mode) != utf8_validator::result::valid) return;
		std::unique_lock lck{m_state->mtx};
		m_state->close(code, reason);
	}

	void websocket::set_on_open(std::function<void(int)> cb) {
		std::unique_lock lck{m_state->mtx};
		m_state->on_open = std::move(cb);
	}

	void websocket::set_on_close(std::function<void(uint16_t, std::string_view)> cb) {
		std::unique_lock lck{m_state->mtx};
		m_state->on_close = std::move(cb);
	}

	void websocket::set_on_message(std::function<void(buffer, bool)> cb) {
		std::unique_lock lck{m_state->mtx};
		m_state->on_message = std::move(cb);
	}

	void websocket::set_on_ping(std::function<void(buffer)> cb) {
		std::unique_lock lck{m_state->mtx};
		m_state->on_ping = std::move(cb);
	}

	void websocket::set_on_pong(std::function<void(buffer)> cb) {
		std::unique_lock lck{m_state->mtx};
		m_state->on_pong = std::move(cb);
	}

	websocket::header_map& websocket::request_headers() noexcept { return m_state->request_headers; };
	websocket::header_map& websocket::response_headers() noexcept { return m_state->response_headers; };
	const websocket::header_map& websocket::request_headers() const noexcept { return m_state->request_headers; };
	const websocket::header_map& websocket::response_headers() const noexcept { return m_state->response_headers; };

	void websocket::set_utf8_validation_mode(utf8_validator::mode m) noexcept { m_state->utf8_mode = m; }
	utf8_validator::mode websocket::utf8_validation_mode() const noexcept { return m_state->utf8_mode; }

	void websocket::send_frame(opcode op, buffer data, std::function<void(bool)> cb) {
		if (m_state->con_state != detail::websocket_state::cs::open) {
			if (cb) cb(false);
			return;
		}
		m_state->send_frame(op, data, std::move(cb));
	}

	detail::websocket_state::cs detail::websocket_state::state_transition(cs newstate) {
		auto old = con_state.exchange(newstate);
		constexpr const char* states[] = {"init", "connect", "handshake", "open", "client_close", "server_close", "closed"};
		if (client.get_handle().is_verbose())
			printf("* curl::websocket %p statechange %s => %s\n", this, states[static_cast<uint32_t>(old)], states[static_cast<uint32_t>(newstate)]);
		return old;
	}

	void detail::websocket_state::raw_on_connect(int code) {
		std::unique_lock lck{mtx};
		state_transition(cs::handshake);
		ref<websocket_state> state(this);
		if (code != 0) {
			if (auto cb = on_open; cb) cb(code);
			return;
		}

		async_scope.launch([](ref<websocket_state> state) -> task<void> {
			std::string buffer;
			try {
				// clang-format off
                    std::string request =
                        "GET " + state->last_url.path_query() + " HTTP/1.1\r\n"
                        "Host: " + state->last_url.host() + (state->last_url.port() > 0 ? (":" + std::to_string(state->last_url.port())) : "") + "\r\n";
				// clang-format on
				for (auto& e : state->request_headers) {
					request += e.first + ": " + e.second + "\r\n";
				}
				request += "\r\n";
				co_await state->client.send_all(request.data(), request.size());

				std::string header;
				bool header_ok = co_await read_until(state->client, "\r\n\r\n", header, buffer);
				if (!header_ok) throw exception(CURLE_RECV_ERROR);
				auto lines = string_split(header, std::string_view("\r\n"));
				if (lines.empty()) throw exception(CURLE_WEIRD_SERVER_REPLY);
				auto status_line = lines[0];
				size_t pos = status_line.find(' ');
				if (pos == std::string::npos) throw exception(CURLE_WEIRD_SERVER_REPLY);
				//http_version = status_line.substr(0, pos);
				status_line = status_line.substr(status_line.find_first_not_of(' ', pos));
				pos = status_line.find(' ');
				if (pos == std::string::npos) throw exception(CURLE_WEIRD_SERVER_REPLY);
				auto status_code = std::stoul(std::string(status_line.substr(0, pos)));
				if (status_code < 100 || status_code > 600) throw exception(CURLE_WEIRD_SERVER_REPLY);
				status_line = status_line.substr(status_line.find_first_not_of(' ', pos));

				lines.erase(lines.begin());
				for (auto e : lines) {
					auto parts = string_split(e, std::string_view(":"));
					if (parts.size() != 2) continue;
					string_trim(parts[0]);
					string_trim(parts[1]);
					state->response_headers.emplace(parts[0], parts[1]);
				}

				auto accept_it = state->response_headers.find("sec-websocket-accept");
				if (status_code != 101 || accept_it == state->response_headers.end() || state->handshake_nonce != accept_it->second)
					throw exception(CURLE_HTTP_RETURNED_ERROR);
			} catch (const exception& e) {
				if (auto cb = state->on_open; cb) cb(e.code());
				co_return;
			} catch (...) {
				if (auto cb = state->on_open; cb) cb(CURLE_FUNCTION_NOT_FOUND);
				co_return;
			}
			if (auto cb = state->on_open; cb) cb(0);
			state->state_transition(cs::open);
			state->parser_data = std::move(buffer);
			// Dummy call to data event to process frames that might have been read together with the handshake
			state->raw_on_data_available(false);
			state->client.set_on_data_available([parent = state.get()](bool cancelled) { return parent->raw_on_data_available(cancelled); });
			state->client.pause_receive(false);
		}(state));
		send_should_exit = false;
		async_scope.launch([](ref<detail::websocket_state> state) -> task<void> {
			while (!state->send_should_exit) {
				while (true) {
					auto element = state->send_queue.pop();
					if (!element) {
						co_await state->send_event.wait(&state->client.get_executor());
						state->send_event.reset();
						break;
					}
					auto res = co_await state->client.send_all(element->first.data(), element->first.size());
					if (element->second) element->second(res == element->first.size());
				}
			}
			while (true) {
				auto e = state->send_queue.pop();
				if (!e) break;
				if (e->second) e->second(false);
			}
			if (state->client.is_connected()) co_await state->client.disconnect();
		}(state));
	}

	void detail::websocket_state::raw_on_disconnect() {
		std::unique_lock lck{mtx};
		if (auto old = state_transition(cs::closed); old == cs::closed || old == cs::server_close) return;
		if (auto cb = on_close; cb) cb(1006, "Connection lost");
	}

	tcp_client::callback_result detail::websocket_state::raw_on_data_available(bool cancelled) {
		// Note: parser_data is never used outside this piece of code and raw_on_data_available is never called concurrently
		//       since it runs on the curl executor, so we can afford to not lock here.
		if (cancelled) {
			raw_on_disconnect();
			return tcp_client::callback_result::clear;
		}
		auto old_size = parser_data.size();
		parser_data.resize(old_size + 64 * 1024);
		auto res = client.recv_raw(parser_data.data() + old_size, parser_data.size() - old_size);
		if (res == 0) {
			raw_on_disconnect();
			return tcp_client::callback_result::clear;
		} else if (res < 0) {
			// No additional data
			res = 0;
		}
		assert(res < (parser_data.size() - old_size));
		parser_data.resize(old_size + res);

		// Parse the data if there's any
		while (parser_data.size() >= parser_wanted_size) {
			auto code = static_cast<opcode>(parser_data[0]);
			bool is_masked = parser_data[1] & 0x80;
			uint64_t payload_len = parser_data[1] & 0x7f;
			size_t header_len = 2 + (is_masked ? 4 : 0) + (payload_len >= 126 ? (payload_len == 127 ? 8 : 2) : 0);
			parser_wanted_size = (std::max)(parser_wanted_size, header_len);
			if (parser_data.size() < parser_wanted_size) continue;
			if (payload_len == 126) {
				payload_len = get_be<uint16_t>(parser_data.data() + 2);
			} else if (payload_len == 127) {
				payload_len = get_be<uint64_t>(parser_data.data() + 2);
			}
			parser_wanted_size = (std::max)(parser_wanted_size, header_len + payload_len);
			if (parser_data.size() < parser_wanted_size) continue;
			uint32_t mask = is_masked ? get_be<uint32_t>(parser_data.data() + header_len - 4) : 0;
			handle_frame(code, parser_data.substr(header_len, payload_len));
			parser_wanted_size = 2;
			parser_data.erase(parser_data.begin(), parser_data.begin() + header_len + payload_len);
		}
		return tcp_client::callback_result::none;
	}

	void detail::websocket_state::handle_frame(opcode code, std::string payload) {
		std::unique_lock lck{mtx};
		if (auto s = con_state.load(); s == cs::closed || s == cs::server_close) return;

		// RSV Bits should be zero
		if (static_cast<uint8_t>(code) & 0x70) return this->close(1002, "Protocol error");

		switch (clean_op(code)) {
		case opcode::continuation: this->handle_continuation_frame(code, std::move(payload)); break;
		case opcode::text: this->handle_text_frame(code, std::move(payload)); break;
		case opcode::binary: this->handle_binary_frame(code, std::move(payload)); break;
		case opcode::close: this->handle_close_frame(code, std::move(payload)); break;
		case opcode::ping: this->handle_ping_frame(code, std::move(payload)); break;
		case opcode::pong: this->handle_pong_frame(code, std::move(payload)); break;
		default: close(1002, "Protocol error"); break;
		}
	}

	void detail::websocket_state::handle_close_frame(opcode op, std::string payload) {
		uint16_t code = 1000;
		std::string_view reason = payload;
		if (reason.size() >= 2) {
			code = get_be<uint16_t>(reason.data());
			reason.remove_prefix(2);
		}

		if (con_state == cs::client_close) {
			// This is a reply to client initiated close
			if (auto cb = on_close; cb) cb(code, reason);
			state_transition(cs::closed);
			return;
		} else {
			state_transition(cs::server_close);
			// Control frames may never be fragmented or have multibyte length
			if (!(op & opcode::fin) || payload.size() > 125 || payload.size() == 1) { return this->close(1002, "Protocol error"); }
			// Check if payload is valid utf8
			if (utf8_validator{}(reason, utf8_mode) != utf8_validator::result::valid) { return this->close(1007, "Invalid utf-8"); }

			if (code < 1000 || code == 1004 || code == 1005 || code == 1006 || (code >= 1015 && code < 3000) || code >= 5000) {
				return this->close(1002, "Protocol error");
			}

			// Close seems valid, send the same code back
			close(code, reason);
		}
	}

	void detail::websocket_state::handle_ping_frame(opcode op, std::string payload) {
		// Control frames may never be fragmented or have multibyte length
		if (!(op & opcode::fin) || payload.size() > 125) { return this->close(1002, "Protocol error"); }
		if (auto cb = on_ping; cb)
			cb(as_bytes(payload));
		else
			this->send_frame(opcode::fin | opcode::pong, as_bytes(payload), [state = ref<websocket_state>(this)](bool) {});
	}

	void detail::websocket_state::handle_pong_frame(opcode op, std::string payload) {
		// Control frames may never be fragmented or have multibyte length
		if (!(op & opcode::fin) || payload.size() > 125) { return this->close(1002, "Protocol error"); }
		if (auto cb = on_pong; cb) cb(as_bytes(payload));
	}

	void detail::websocket_state::handle_text_frame(opcode op, std::string payload) {
		// The fragment buffer should be empty
		if (fragment_code != opcode::continuation) return close(1002, "Protocol error");
		if (!(op & opcode::fin)) {
			// Fragment start
			fragment_code = opcode::text;
			fragment_payload = std::move(payload);
			std::string_view sv{fragment_payload};
			auto last_valid = sv.cbegin();
			if (utf8_validator{}(sv, utf8_mode, &last_valid) == utf8_validator::result::invalid) return close(1007, "Invalid utf8");
			fragment_validate_offset = std::distance(sv.begin(), last_valid);
		} else { // Unfragmented message
			if (utf8_validator{}(payload, utf8_mode) != utf8_validator::result::valid) return close(1007, "Invalid utf8");
			if (auto cb = on_message; cb) cb(as_bytes(payload), false);
		}
	}

	void detail::websocket_state::handle_binary_frame(opcode op, std::string payload) {
		// The fragment buffer should be empty
		if (fragment_code != opcode::continuation) return close(1002, "Protocol error");
		if (!(op & opcode::fin)) {
			// Fragment start
			fragment_code = opcode::binary;
			fragment_payload = std::move(payload);
			fragment_validate_offset = 0;
		} else { // Unfragmented message
			if (auto cb = on_message; cb) cb(as_bytes(payload), true);
		}
	}

	void detail::websocket_state::handle_continuation_frame(opcode op, std::string payload) {
		// The fragment buffer should not be empty
		if (fragment_code == opcode::continuation) return close(1002, "Protocol error");
		fragment_payload.append(payload);
		if (!!(op & opcode::fin)) { // We are done
			bool binary = fragment_code == opcode::binary;
			fragment_code = opcode::continuation;
			payload = std::move(fragment_payload);
			if (!binary) {
				std::string_view sv{payload};
				sv.remove_prefix(fragment_validate_offset);
				if (utf8_validator{}(sv, utf8_mode) != utf8_validator::result::valid) return close(1007, "Invalid utf8");
			}
			if (auto cb = on_message; cb) cb(as_bytes(payload), binary);
		} else if (fragment_code != opcode::binary) {
			// Fail fast if received utf8 causes invalid sequences
			std::string_view sv{fragment_payload};
			sv.remove_prefix(fragment_validate_offset);
			auto last_valid = sv.cbegin();
			if (utf8_validator{}(sv, utf8_mode, &last_valid) == utf8_validator::result::invalid) return close(1007, "Invalid utf8");
			fragment_validate_offset += std::distance(sv.begin(), last_valid);
		}
	}

	void detail::websocket_state::connect(const uri& url) {
		std::unique_lock lck{mtx};
		state_transition(cs::connect);
		ref<websocket_state> state(this);
		last_url = url;
		client.connect(url.host(), url.port(), url.scheme() == "wss", [state](int result) mutable {
			state->raw_on_connect(result);
			state.reset();
		});
	}

	void detail::websocket_state::close(uint16_t code, std::string_view reason) {
		if (con_state == cs::closed || con_state == cs::client_close) return;
		std::function<void(bool)> cb;
		if (con_state == cs::server_close) {
			// CLOSE frame was received
			state_transition(cs::closed);
			cb = [cb = on_close, code, reason = std::string(reason), state = ref<websocket_state>(this)](bool) {
				if (cb) cb(code, reason);
			};
		} else {
			cb = [state = ref<websocket_state>(this)](bool) {};
		}
		std::string payload;
		payload.resize(sizeof(uint16_t) + reason.size());
		set_be<uint16_t>(payload.data(), code);
		std::memcpy(payload.data() + 2, reason.data(), reason.size());
		send_frame(opcode::fin | opcode::close, as_bytes(payload), std::move(cb));
	}

	void detail::websocket_state::send_frame(opcode op, const websocket::buffer buf, std::function<void(bool)> cb) {
		if (clean_op(op) == opcode::close && con_state == cs::open) {
			// If we are the first to close, call on_close on disconnect/fin reply
			state_transition(cs::client_close);
		}
		std::pair<std::string, std::function<void(bool)>> data;
		data.second = cb;
		size_t header_len = 2 + 4;
		if (buf.size_bytes() > (std::numeric_limits<uint16_t>::max)())
			header_len += sizeof(uint64_t);
		else if (buf.size_bytes() > 125)
			header_len += sizeof(uint16_t);
		data.first.resize(header_len + buf.size_bytes());
		memcpy(data.first.data() + header_len, buf.data(), buf.size_bytes());
		data.first[0] = static_cast<uint8_t>(op);
		if (buf.size_bytes() > (std::numeric_limits<uint16_t>::max)()) {
			data.first[1] = 127;
			set_be<uint64_t>(data.first.data() + 2, buf.size_bytes());
		} else if (buf.size_bytes() > 125) {
			data.first[1] = 126;
			set_be<uint16_t>(data.first.data() + 2, buf.size_bytes());
		} else {
			data.first[1] = buf.size_bytes();
		}
		data.first[1] |= 0x80;

		auto masking_key = data.first.data() + header_len - 4;
		std::uniform_int_distribution<short> dist((std::numeric_limits<char>::min)(), (std::numeric_limits<char>::max)());
		std::generate(masking_key, masking_key + 4, [dist]() mutable -> char { return dist(g_random); });
		for (size_t i = 0; i < buf.size_bytes(); i++) {
			data.first[i + header_len] ^= masking_key[i % 4];
		}
		send_queue.emplace(std::move(data));
		send_event.set();
	}

} // namespace asyncpp::curl
