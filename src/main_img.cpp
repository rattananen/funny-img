#include "img/bmp.hpp"
#include "img/png.hpp"

/// @param table higher luminance first
/// @param len length for table;
template<typename T>
char to_char(T c, const std::string& table) {
	return table[static_cast<size_t>(img::luminance(c) * table.length()) % table.length()];
}

void stream_error(std::ostream& err, const std::error_code& ec) {
	err << '[' << ec.category().name() << ':' << ec.value() << ']' << ' ' << ec.message();
}

int cmd_convert_png(const std::string& in, std::ostream& os, std::ostream& err, const std::string& table) {
	using namespace img::png;

	PngFileReader re{in};
	if (auto ec = re.fetch_meta()) {
		stream_error(err, ec);
		return 1;
	}
	auto decoder = re.decoder();
	auto row_gen = decoder();

	while (row_gen) {
		auto row_view_ptr = row_gen();
		for (auto p : *row_view_ptr) {
			os << to_char(p, table);
		}
		os << "\n";
	}
	return 0;
}

/// @return error code
int cmd_convert_bmp(const std::string& in, std::ostream& os, std::ostream& err, const std::string& table) {
	using namespace img::bmp;
	BmpFileReader freader{in};
	if (auto ec = freader.fetch_meta()) {
		stream_error(err, ec);
		return 1;
	}
	
	for (auto& row : freader.view()) {
		for (auto p : row) {
			os << to_char(p, table);
		}
		os << '\n';
	}
	return 0;
}

int cmd_convert(const std::string& in, std::ostream& os, std::ostream& err, const std::string& table) {
	int ec = cmd_convert_bmp(in, os, err, table);
	if (ec == 0) {
		return 0;
	}

	return cmd_convert_png(in, os, err, table);
}

constexpr auto help_text =
"usage:\n"
" funny_img <image path> [char table (default=ABCDEFG)]\n"
"  - accept only some bmp and some png format\n"
"  - output will send to stdout (redirect by POSIX 1>)\n"
"  - error/info will send to stderr (redirect by POSIX 2>)\n";

int main(int argc, const char** argv)
{
	if (argc != 2 && argc != 3)
	{
		std::cerr << "invalid arguments\n"
			<< help_text;
		return 1;
	}
	if (argc == 2) {
		return cmd_convert(argv[1], std::cout, std::cerr, std::string{"ABCDEFG"});
	}
	if (argc == 3) {
		return cmd_convert(argv[1], std::cout, std::cerr, argv[2]);
	}
	return 0;
}
