#include "img/bmp.hpp"


/// @param table higher luminance first
/// @param len length for table;
char to_char(img::Rgb24 c, const char* table, size_t len) {

	return table[static_cast<size_t>(img::luminance(c) * len) % len];
}

/// @return error code
int cmd_convert(const std::string& in, std::ostream& os, const char* table = "ABCDEFG") {
	using namespace img::bmp;
	BmpFileReader freader{in};
	if (auto ec = freader.fetch_meta()) {
		std::cerr << std::format("[{}:{}] {}\n", ec.category().name(), ec.value(),ec.message());
		return 1;
	}
	
	auto len = std::char_traits<char>::length(table);
	for (auto& row : freader.view()) {
		for (auto p : row) {
			os << to_char(p, table, len);
		}
		os << '\n';
	}
	return 0;
}

constexpr auto help_text =
"usage:\n"
" funny_img <input image path> [char table (default=ABCDEFG)]\n";

int main(int argc, const char** argv)
{
	if (argc != 2 && argc != 3)
	{
		std::cerr << "invalid arguments\n"
			<< help_text;
		return 1;
	}
	if (argc == 2) {
		return cmd_convert(argv[1], std::cout);
	}
	if (argc == 3) {
		return cmd_convert(argv[1], std::cout, argv[2]);
	}
	return 0;
}
