#include <asyncpp/curl/base64.h>
#include <openssl/evp.h>
#include <stdexcept>

namespace asyncpp::curl {

	std::string base64::encode(const std::string_view data) {
		std::string res;
		const int pl = 4 * ((data.size() + 2) / 3);
		res.resize(pl + 1);
		const int ol = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(res.data()), reinterpret_cast<const unsigned char*>(data.data()), data.size());
		if (pl != ol) throw std::logic_error("base64 size missmatch");
		if (ol < static_cast<int>(res.size())) res.resize(ol);
		return res;
	}

	std::string base64::decode(const std::string_view data) {
		size_t padding = 0;
		while (padding < data.size() && data[(data.size() - 1) - padding] == '=')
			padding++;
		if (padding > 2) throw std::runtime_error("failed to decode base64");
		std::string res;
		res.resize(((3 * data.size()) / 4) + 1);
		const int ol = EVP_DecodeBlock(reinterpret_cast<unsigned char*>(res.data()), reinterpret_cast<const unsigned char*>(data.data()), data.size());
		if (ol == -1) throw std::runtime_error("failed to decode base64");
		res.resize(ol - padding);
		return res;
	}

	std::string base64url::encode(const std::string_view data) {
		auto res = base64::encode(data);
		for (auto& e : res) {
			if (e == '+')
				e = '-';
			else if (e == '/')
				e = '_';
		}
		auto pos = res.find('=');
		if (pos != std::string::npos) res.resize(pos);
		return res;
	}

	std::string base64url::decode(const std::string_view data) {
		std::string d{data};
		for (auto& e : d) {
			if (e == '-')
				e = '+';
			else if (e == '_')
				e = '/';
		}
		d.resize((d.size() + 3) & ~0x3, '=');
		return base64::decode(d);
	}

} // namespace asyncpp::curl