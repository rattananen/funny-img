#pragma once

#include <inttypes.h>
#include <istream>

namespace img {
	struct Rgb24 {
		uint8_t b;
		uint8_t g;
		uint8_t r;
	};

	struct Rgba32 {
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;

		uint8_t& operator[](size_t i) {
			switch (i)
			{
			default:
			case 0:
				return r;
			case 1:
				return g;
			case 2:
				return b;
			case 3:
				return a;
			}
		}
	};

	template<typename T>
	double luminance(T c) {
		return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
	}

	/*Rgba32 operator+(Rgba32 a, Rgba32 b) {
		Rgba32 ret;
		ret.r = a.r + b.r;
		ret.g = a.g + b.g;
		ret.b = a.b + b.b;
		ret.a = a.a + b.a;
		return ret;
	}

	Rgba32 operator-(Rgba32 a, Rgba32 b) {
		Rgba32 ret;
		ret.r = a.r - b.r;
		ret.g = a.g - b.g;
		ret.b = a.b - b.b;
		ret.a = a.a - b.a;
		return ret;
	}

	Rgba32 operator*(Rgba32 a, Rgba32 b) {
		Rgba32 ret;
		ret.r = a.r * b.r;
		ret.g = a.g * b.g;
		ret.b = a.b * b.b;
		ret.a = a.a * b.a;
		return ret;
	}

	Rgba32 operator/(Rgba32 a, Rgba32 b) {
		Rgba32 ret;
		ret.r = a.r / b.r;
		ret.g = a.g / b.g;
		ret.b = a.b / b.b;
		ret.a = a.a / b.a;
		return ret;
	}

	Rgba32 operator/(Rgba32 a, int b) {
		Rgba32 ret;
		ret.r = a.r / b;
		ret.g = a.g / b;
		ret.b = a.b / b;
		ret.a = a.a / b;
		return ret;
	}*/
}