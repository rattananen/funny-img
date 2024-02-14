#include "img/png.hpp"

int main() {
	using namespace img::png;
	PngFileReader re{"test.png"};
	if (auto ec = re.fetch_meta()) {
		std::cerr << std::format("[{}:{}] {}\n", ec.category().name(), ec.value(), ec.message());
		return 1;
	}
	re.info_to(std::cout);

	re.read_pixel();
	return 0;
}