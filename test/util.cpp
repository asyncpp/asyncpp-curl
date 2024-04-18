#include <asyncpp/curl/util.h>
#include <gtest/gtest.h>

using namespace asyncpp::curl;
namespace {

	using result = utf8_validator::result;
	constexpr const char* result_names[] = {"invalid", "valid", "valid_incomplete"};
	// https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
	// Test, normal result, strict result, pedantic result, extreme result
	constexpr std::tuple<std::string_view, result, result, result, result> test_cases[]{
		{"\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce\xb5", result::valid, result::valid, result::valid, result::valid},
		// 2.1 (1)
		{{"\x00", 1}, result::valid, result::valid, result::valid, result::valid},
		{"\xC2\x80", result::valid, result::valid, result::valid, result::valid},
		{"\xe0\xa0\x80", result::valid, result::valid, result::valid, result::valid},
		{"\xf0\x90\x80\x80", result::valid, result::valid, result::valid, result::valid},
		{"\xf8\x88\x80\x80\x80", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xfc\x84\x80\x80\x80\x80", result::valid, result::invalid, result::invalid, result::invalid},
		// 2.2 (7)
		{"\x7f", result::valid, result::valid, result::valid, result::valid},
		{"\xdf\xbf", result::valid, result::valid, result::valid, result::valid},
		{"\xef\xbf\xbf", result::valid, result::valid, result::valid, result::invalid},
		{"\xf7\xbf\xbf\xbf", result::valid, result::valid, result::invalid, result::invalid},
		{"\xfb\xbf\xbf\xbf\xbf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xfd\xbf\xbf\xbf\xbf\xbf", result::valid, result::invalid, result::invalid, result::invalid},
		// 2.3 (13)
		{"\xed\x9f\xbf", result::valid, result::valid, result::valid, result::valid},
		{"\xee\x80\x80", result::valid, result::valid, result::valid, result::valid},
		{"\xef\xbf\xbd", result::valid, result::valid, result::valid, result::valid},
		{"\xf4\x8f\xbf\xbf", result::valid, result::valid, result::valid, result::invalid},
		{"\xf4\x90\x80\x80", result::valid, result::valid, result::invalid, result::invalid},
		// 3.1 (18)
		{"\x80", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\xbf", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\x80\xbf", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\x80\xbf\x80", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\x80\xbf\x80\xbf", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\x80\xbf\x80\xbf\x80", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\x80\xbf\x80\xbf\x80\xbf", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\x80\xbf\x80\xbf\x80\xbf\x80", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f\xa0\xa1\xa2\xa3\xa4"
		 "\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf",
		 result::invalid, result::invalid, result::invalid, result::invalid},
		// 3.2 (27)
		{"\xc0\x20\xc1\x20\xc2\x20\xc3\x20\xc4\x20\xc5\x20\xc6\x20\xc7\x20\xc8\x20\xc9\x20\xca\x20\xcb\x20\xcc\x20\xcd\x20\xce\x20\xcf\x20\xd0\x20\xd1\x20\xd2"
		 "\x20\xd3\x20\xd4\x20\xd5\x20\xd6\x20\xd7\x20\xd8\x20\xd9\x20\xda\x20\xdb\x20\xdc\x20\xdd\x20\xde\x20\xdf\x20",
		 result::invalid, result::invalid, result::invalid, result::invalid},
		{"\xe0\x20\xe1\x20\xe2\x20\xe3\x20\xe4\x20\xe5\x20\xe6\x20\xe7\x20\xe8\x20\xe9\x20\xea\x20\xeb\x20\xec\x20\xed\x20\xee\x20\xef\x20", result::invalid,
		 result::invalid, result::invalid, result::invalid},
		{"\xf0\x20\xf1\x20\xf2\x20\xf3\x20\xf4\x20\xf5\x20\xf6\x20\xf7\x20", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\xf8\x20\xf9\x20\xfa\x20\xfb\x20", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\xfc\x20\xfd\x20", result::invalid, result::invalid, result::invalid, result::invalid},
		// 3.3 (32)
		{"\xc0", result::valid_incomplete, result::valid_incomplete, result::valid_incomplete, result::valid_incomplete},
		{"\xe0\x80", result::valid_incomplete, result::valid_incomplete, result::valid_incomplete, result::valid_incomplete},
		{"\xf0\x80\x80", result::valid_incomplete, result::valid_incomplete, result::valid_incomplete, result::valid_incomplete},
		{"\xf8\x80\x80\x80", result::valid_incomplete, result::invalid, result::invalid, result::invalid},
		{"\xfc\x80\x80\x80\x80", result::valid_incomplete, result::invalid, result::invalid, result::invalid},
		{"\xdf", result::valid_incomplete, result::valid_incomplete, result::valid_incomplete, result::valid_incomplete},
		{"\xef\xbf", result::valid_incomplete, result::valid_incomplete, result::valid_incomplete, result::valid_incomplete},
		{"\xf7\xbf\xbf", result::valid_incomplete, result::valid_incomplete, result::invalid, result::invalid},
		{"\xfb\xbf\xbf\xbf", result::valid_incomplete, result::invalid, result::invalid, result::invalid},
		{"\xfd\xbf\xbf\xbf\xbf", result::valid_incomplete, result::invalid, result::invalid, result::invalid},
		// 3.4 (42)
		{"\xc0\xe0\x80\xf0\x80\x80\xf8\x80\x80\x80\xfc\x80\x80\x80\x80\xdf\xef\xbf\xf7\xbf\xbf\xfb\xbf\xbf\xbf\xfd\xbf\xbf\xbf\xbf", result::invalid,
		 result::invalid, result::invalid, result::invalid},
		// 3.5 (43)
		{"\xfe", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\xff", result::invalid, result::invalid, result::invalid, result::invalid},
		{"\xfe\xfe\xff\xff", result::invalid, result::invalid, result::invalid, result::invalid},
		// 4.1 (46)
		{"\xc0\xaf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xe0\x80\xaf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xf0\x80\x80\xaf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xf8\x80\x80\x80\xaf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xfc\x80\x80\x80\x80\xaf", result::valid, result::invalid, result::invalid, result::invalid},
		// 4.2 (51)
		{"\xc1\xbf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xe0\x9f\xbf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xf0\x8f\xbf\xbf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xf8\x87\xbf\xbf\xbf", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xfc\x83\xbf\xbf\xbf\xbf", result::valid, result::invalid, result::invalid, result::invalid},
		// 4.3 (56)
		{"\xc0\x80", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xe0\x80\x80", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xf0\x80\x80\x80", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xf8\x80\x80\x80\x80", result::valid, result::invalid, result::invalid, result::invalid},
		{"\xfc\x80\x80\x80\x80\x80", result::valid, result::invalid, result::invalid, result::invalid},
		// 5.1 (61)
		{"\xed\xa0\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xad\xbf", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xae\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xaf\xbf", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xb0\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xbe\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xbf\xbf", result::valid, result::valid, result::invalid, result::invalid},
		// 5.2 (68)
		{"\xed\xa0\x80\xed\xb0\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xa0\x80\xed\xbf\xbf", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xad\xbf\xed\xb0\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xad\xbf\xed\xbf\xbf", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xae\x80\xed\xb0\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xae\x80\xed\xbf\xbf", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xaf\xbf\xed\xb0\x80", result::valid, result::valid, result::invalid, result::invalid},
		{"\xed\xaf\xbf\xed\xbf\xbf", result::valid, result::valid, result::invalid, result::invalid},
		// 5.3 (76)
		{"\xef\xbf\xbe", result::valid, result::valid, result::valid, result::invalid},
		{"\xef\xbf\xbf", result::valid, result::valid, result::valid, result::invalid},
		{"\xef\xb7\x90\xef\xb7\x91\xef\xb7\x92\xef\xb7\x93\xef\xb7\x94\xef\xb7\x95\xef\xb7\x96\xef\xb7\x97\xef\xb7\x98\xef\xb7\x99\xef\xb7\x9a\xef\xb7\x9b\xef"
		 "\xb7\x9c\xef\xb7\x9d\xef\xb7\x9e\xef\xb7\x9f\xef\xb7\xa0\xef\xb7\xa1\xef\xb7\xa2\xef\xb7\xa3\xef\xb7\xa4\xef\xb7\xa5\xef\xb7\xa6\xef\xb7\xa7\xef\xb7"
		 "\xa8\xef\xb7\xa9\xef\xb7\xaa\xef\xb7\xab\xef\xb7\xac\xef\xb7\xad\xef\xb7\xae\xef\xb7\xaf",
		 result::valid, result::valid, result::invalid, result::invalid},
		{"\xf0\x9f\xbf\xbe\xf0\x9f\xbf\xbf\xf0\xaf\xbf\xbe\xf0\xaf\xbf\xbf\xf0\xbf\xbf\xbe\xf0\xbf\xbf\xbf\xf1\x8f\xbf\xbe\xf1\x8f\xbf\xbf\xf1\x9f\xbf\xbe\xf1"
		 "\x9f\xbf\xbf\xf1\xaf\xbf\xbe\xf1\xaf\xbf\xbf\xf1\xbf\xbf\xbe\xf1\xbf\xbf\xbf\xf2\x8f\xbf\xbe\xf2\x8f\xbf\xbf\xf2\x9f\xbf\xbe\xf2\x9f\xbf\xbf\xf2\xaf"
		 "\xbf\xbe\xf2\xaf\xbf\xbf\xf2\xbf\xbf\xbe\xf2\xbf\xbf\xbf\xf3\x8f\xbf\xbe\xf3\x8f\xbf\xbf\xf3\x9f\xbf\xbe\xf3\x9f\xbf\xbf\xf3\xaf\xbf\xbe\xf3\xaf\xbf"
		 "\xbf\xf3\xbf\xbf\xbe\xf3\xbf\xbf\xbf\xf4\x8f\xbf\xbe\xf4\x8f\xbf\xbf",
		 result::valid, result::valid, result::valid, result::invalid},

		// Other
		{"\xce\xba\xe1\xbd\xb9\xcf\x83\xce\xbc\xce\xb5\xf4\x90\x80\x80\x65\x64\x69\x74\x65\x64", result::valid, result::valid, result::invalid,
		 result::invalid},
	};

} // namespace

TEST(ASYNCPP_CURL, Utf8CheckNormal) {
	constexpr size_t size = sizeof(test_cases) / sizeof(test_cases[0]);
	for (size_t i = 0; i < size; i++) {
		auto& t = test_cases[i];
		auto res = utf8_validator{}(std::get<0>(t), utf8_validator::mode::normal);
		auto wanted = std::get<1>(t);
		ASSERT_EQ(res, wanted) << "Failed test " << i << " expected=" << result_names[static_cast<int>(wanted)]
							   << " got=" << result_names[static_cast<int>(res)];
	}
}

TEST(ASYNCPP_CURL, Utf8CheckStrict) {
	constexpr size_t size = sizeof(test_cases) / sizeof(test_cases[0]);
	for (size_t i = 0; i < size; i++) {
		auto& t = test_cases[i];
		auto res = utf8_validator{}(std::get<0>(t), utf8_validator::mode::strict);
		auto wanted = std::get<2>(t);
		ASSERT_EQ(res, wanted) << "Failed test " << i << " expected=" << result_names[static_cast<int>(wanted)]
							   << " got=" << result_names[static_cast<int>(res)];
	}
}

TEST(ASYNCPP_CURL, Utf8CheckPedantic) {
	constexpr size_t size = sizeof(test_cases) / sizeof(test_cases[0]);
	for (size_t i = 0; i < size; i++) {
		auto& t = test_cases[i];
		auto res = utf8_validator{}(std::get<0>(t), utf8_validator::mode::pedantic);
		auto wanted = std::get<3>(t);
		ASSERT_EQ(res, wanted) << "Failed test " << i << " expected=" << result_names[static_cast<int>(wanted)]
							   << " got=" << result_names[static_cast<int>(res)];
	}
}

TEST(ASYNCPP_CURL, Utf8CheckExtreme) {
	constexpr size_t size = sizeof(test_cases) / sizeof(test_cases[0]);
	for (size_t i = 0; i < size; i++) {
		auto& t = test_cases[i];
		auto res = utf8_validator{}(std::get<0>(t), utf8_validator::mode::extreme);
		auto wanted = std::get<4>(t);
		ASSERT_EQ(res, wanted) << "Failed test " << i << " expected=" << result_names[static_cast<int>(wanted)]
							   << " got=" << result_names[static_cast<int>(res)];
	}
}
