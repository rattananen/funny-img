#pragma once

#include <inttypes.h>
#include <istream>

namespace img {
	struct Rgb24 {
		uint8_t b;
		uint8_t g;
		uint8_t r;
	};

	

	double luminance(const Rgb24 c) {
		return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
	}
}