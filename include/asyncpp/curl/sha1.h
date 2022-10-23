#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace asyncpp::curl {
	class sha1 {
		uint32_t m_state[5];
		uint32_t m_count[2]{};
		unsigned char m_buffer[64]{};

	public:
		sha1() noexcept;
		void update(const void* data, size_t len) noexcept;
		void finish(void* digest) noexcept;

		void operator()(const void* data, size_t len) noexcept { update(data, len); }

		static std::string hash(const void* data, size_t len) {
			sha1 h;
			h(data, len);
			std::string res;
			res.resize(20);
			h.finish(res.data());
			return res;
		}
		static std::string hash(const std::string& data) { return hash(data.data(), data.size()); }

	private:
	};
} // namespace asyncpp::curl
