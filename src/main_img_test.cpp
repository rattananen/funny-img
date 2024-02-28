#include "img/png.hpp"

void info(img::Rgba32 c) {
	std::cout << "[" 
		<< static_cast<int>(c.r) << " " 
		<< static_cast<int>(c.g) << " " 
		<< static_cast<int>(c.b) << "] ";
}

int main() {
	using namespace img::png;
	PngFileReader re{"test.png"};
	if (auto ec = re.fetch_meta()) {
		std::cerr << std::format("[{}:{}] {}\n", ec.category().name(), ec.value(), ec.message());
		return 1;
	}

	auto decoder = re.decoder();
	auto row_gen = decoder();

	while (row_gen) {
		auto row_view_ptr = row_gen();
		for (auto pixel: *row_view_ptr) {
			info(pixel);
		}
		std::cout << "\n";
	}
	return 0;
}