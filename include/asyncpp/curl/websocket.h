#pragma once
#include <asyncpp/detail/std_import.h>
#include <asyncpp/ref.h>
#include <asyncpp/launch.h>

#include <asyncpp/curl/exception.h>
#include <asyncpp/curl/tcp_client.h>
#include <asyncpp/curl/uri.h>
#include <asyncpp/curl/util.h>

#include <functional>
#include <span>

namespace asyncpp::curl {
	class executor;
	namespace detail {
		struct websocket_state;
		void refcounted_add_ref(const websocket_state* ptr) noexcept;
		void refcounted_remove_ref(const websocket_state* ptr) noexcept;
	}
	class websocket {
	public:
		enum class opcode : uint8_t {
			continuation = 0x00,
			text,
			binary,
			close = 0x08,
			ping,
			pong,

			fin = 0x80,
		};
		using header_map = std::multimap<std::string, std::string, case_insensitive_less>;
		using buffer = std::span<const std::byte>;

		websocket(executor& e);
		websocket();
		~websocket();

		void connect(const uri& url);
		void close(uint16_t code, std::string_view reason);
		bool is_connected() const noexcept;

		void send_frame(opcode op, buffer data, std::function<void(bool)> cb);
		void send(buffer data, bool binary, std::function<void(bool)> cb);
		void send_text(std::string_view sv, std::function<void(bool)> cb);
		void send_binary(buffer data, std::function<void(bool)> cb);
		void send_ping(buffer data, std::function<void(bool)> cb);
		void send_pong(buffer data, std::function<void(bool)> cb);

		void set_on_open(std::function<void(int)> cb);
		void set_on_close(std::function<void(uint16_t, std::string_view)> cb);
		void set_on_message(std::function<void(buffer, bool)> cb);
		void set_on_ping(std::function<void(buffer)> cb);
		void set_on_pong(std::function<void(buffer)> cb);
		void set_on_frame(std::function<void(opcode, buffer)> cb);

		header_map& request_headers() noexcept;
		header_map& response_headers() noexcept;
		const header_map& request_headers() const noexcept;
		const header_map& response_headers() const noexcept;

		void set_utf8_validation_mode(utf8_validator::mode m) noexcept;
		utf8_validator::mode utf8_validation_mode() const noexcept;

	private:
		ref<detail::websocket_state> m_state;
	};

	inline constexpr websocket::opcode operator|(websocket::opcode lhs, websocket::opcode rhs) noexcept {
		return static_cast<websocket::opcode>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
	}

	inline constexpr websocket::opcode operator&(websocket::opcode lhs, websocket::opcode rhs) noexcept {
		return static_cast<websocket::opcode>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
	}

	inline constexpr bool operator!(websocket::opcode lhs) noexcept { return lhs == websocket::opcode{}; }

	template<typename T, typename Traits, typename Alloc>
	std::span<const std::byte> as_bytes(const std::basic_string<T, Traits, Alloc>& str) noexcept {
		return std::as_bytes(std::span<const T>(str.data(), str.size()));
	}

	template<typename T, typename Traits, typename Alloc>
	std::span<std::byte> as_writable_bytes(std::basic_string<T, Traits, Alloc>& str) noexcept {
		return std::as_bytes(std::span<const T>(str.data(), str.size()));
	}

	template<typename T, typename Traits>
	std::span<const std::byte> as_bytes(std::basic_string_view<T, Traits> str) noexcept {
		return std::as_bytes(std::span<const T>(str.data(), str.size()));
	}

	inline void websocket::send(buffer data, bool binary, std::function<void(bool)> cb) {
		send_frame(opcode::fin | (binary ? opcode::binary : opcode::text), data, std::move(cb));
	}

	inline void websocket::send_binary(buffer data, std::function<void(bool)> cb) {
		send_frame(opcode::fin | opcode::binary, data, std::move(cb));
	}

	inline void websocket::send_text(std::string_view sv, std::function<void(bool)> cb) {
		send_frame(opcode::fin | opcode::text, as_bytes(sv), std::move(cb));
	}

	inline void websocket::send_ping(buffer data, std::function<void(bool)> cb) {
		send_frame(opcode::fin | opcode::ping, data, std::move(cb));
	}

	inline void websocket::send_pong(buffer data, std::function<void(bool)> cb) {
		send_frame(opcode::fin | opcode::pong, data, std::move(cb));
	}

} // namespace asyncpp::curl
