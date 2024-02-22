#pragma once

#include "deflate.hpp"
#include "png_error.hpp"
#include <inttypes.h>
#include <iostream>
#include <array>
#include <fstream>
#include <sstream>
#include <format>


/// https://www.w3.org/TR/png/#13Decompression
namespace img::png {

	/// @brief for changing endian purpose
	template<typename T, size_t SIZE = sizeof(T), size_t MAX_IDX = SIZE - 1>
		requires std::integral<T>
	void consume_swap_bytes(std::istream& is, T& buf) {
		for (int i = 0; i < SIZE; ++i) {
			is.read(&reinterpret_cast<char*>(&buf)[MAX_IDX - i], 1);
		}
	}

	constexpr uint64_t PNG_SIGNATURE = 0x8950'4E47'0D0A'1A0A;

	enum struct ChunkId :uint32_t {
		IHDR = 0x4948'4452,
		IDAT = 0X4944'4154,
		IEND = 0x4945'4E44
	};

	enum struct BitDepth :uint8_t {
		bit1 = 1,
		bit2 = 2,
		bit4 = 4,
		bit8 = 8,
		bit16 = 16,
	};
	enum struct ColorType :uint8_t {
		indexed = 0,
		grayscale = 2,
		grayscale_a = 3,
		truecolor = 4,
		truecolor_a = 6,
	};

	struct Png {
		
		struct IHDR {
			uint32_t width;
			uint32_t height;
			BitDepth bitdetph;
			ColorType color_type;
			uint8_t compress_method;
			uint8_t filter_method;
			bool interlace;
		};

		IHDR ihdr;
	};

	std::string str_info(const Png::IHDR& ihdr) {
		return std::format(
			"width={}\n"
			"height={}\n"
			"bitdetph={}\n"
			"color_type={}\n"
			"compress_method={:d}\n"
			"filter_method={:d}\n"
			"interlace={}\n", 
			ihdr.width,
			ihdr.height,
			static_cast<int>(ihdr.bitdetph),
			static_cast<int>(ihdr.color_type),
			ihdr.compress_method,
			ihdr.filter_method,
			ihdr.interlace);
	}

	std::istream& operator>>(std::istream& is, Png::IHDR& ihdr) {
		consume_swap_bytes(is, ihdr.width);
		consume_swap_bytes(is, ihdr.height);
	
		return is
			.read(reinterpret_cast<char*>(&ihdr.bitdetph), 1)
			.read(reinterpret_cast<char*>(&ihdr.color_type), 1)
			.read(reinterpret_cast<char*>(&ihdr.compress_method), 1)
			.read(reinterpret_cast<char*>(&ihdr.filter_method), 1)
			.read(reinterpret_cast<char*>(&ihdr.interlace), 1);
	}
	struct PngReader {
		
		PngReader(std::istream& in) :m_is{ in }
		{}

		std::error_code read_meta(Png& png)
		{
			m_is.seekg(0); 
			
			uint64_t signature{};
			consume_swap_bytes(m_is, signature);

			std::error_code ec;
			if (signature != PNG_SIGNATURE) {
				ec = PngError::invalid_signature;
				goto fail_return;
			}

			if (!goto_chunk(ChunkId::IHDR)) {
				ec = PngError::invalid_ihdr;
				goto fail_return;
			}
			
			m_is >> png.ihdr; //chunk data

			if (png.ihdr.bitdetph != BitDepth::bit8) {
				ec = PngError::bitdepth_not_support;
				goto fail_return;
			}

			if (png.ihdr.color_type != ColorType::truecolor_a) {
				ec = PngError::color_type_not_support;
				goto fail_return;
			}

			if (png.ihdr.interlace) {
				ec = PngError::interlace_not_support;
				goto fail_return;
			}

			m_is.seekg(4, std::ios::cur);//skip crc and point to next chunk
			
			return ec;

		fail_return:
			m_is.setstate(std::ios::badbit);
			return ec;
		}

		std::error_code read_pixels(Png& png) {
			if (!goto_chunk(ChunkId::IDAT)) {
				return PngError::invalid_idat;
			}
			deflate::Inflater::window_t bytes;
			bytes.reserve(png.ihdr.height * png.ihdr.width * 32 + png.ihdr.height);

			deflate::Inflater decompressor{m_is, bytes };

			deflate::Header header;

			decompressor.consume_head(header);

			if (header.CF != 8 || header.CINFO != 7 || header.FDICT != 0) {
				return PngError::deflate_decompress_fail;
			}

			auto ec = decompressor.decompress();

			if (ec) {
				return PngError::deflate_decompress_fail;
			}
		
			std::cout << "error=" << ec << '\n';
			std::cout << "bytes size=" << bytes.size() << '\n';
	
			return {};
		}

		bool goto_chunk(ChunkId id) {
			while (m_is.good()) {
				consume_swap_bytes(m_is, last_size);
				consume_swap_bytes(m_is, last_id);
				if (last_id == static_cast<uint32_t>(id)) {
					return true;
				}
				if (last_id == static_cast<uint32_t>(ChunkId::IEND)) {
					return false;
				}
				m_is.seekg(last_size + 4, std::ios::cur);//skip crc and point to next chunk
			}
			return false;
		}
		
	private:
		uint32_t last_size = 0;
		uint32_t last_id = 0;
		std::istream& m_is;
	};

	struct PngFileReader {
		PngFileReader(const std::string& _path) :path{ _path }, ifs{ _path ,  std::ios::binary }, reader{ ifs}, png{} {};


		void info_to(std::ostream& os) const {
			os << str_info(png.ihdr);
		}

		std::error_code fetch_meta() {
			if (!ifs.is_open()) {
				return PngError::fail_open_file;
			}
			return reader.read_meta(png);
		}

		std::error_code read_pixel() {
			return reader.read_pixels(png);
		}

		const std::string path;
		Png png;
		PngReader reader;
		std::ifstream ifs;

	};
}