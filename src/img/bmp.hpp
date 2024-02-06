#pragma once

#include "pixel.hpp"
#include <iostream>
#include <vector>
#include <format>
#include <fstream>
#include <format>

namespace img::bmp {

	std::istream& consume_pixel(std::istream& is, Rgb24& pixel)
	{
		return is
			.read(reinterpret_cast<char*>(&pixel.b), 1)
			.read(reinterpret_cast<char*>(&pixel.g), 1)
			.read(reinterpret_cast<char*>(&pixel.r), 1);
	}

	struct Bmp {
		enum struct BitDepth :uint16_t {
			bit1 = 1, bit4 = 4, bit8 = 8,
			bit16 = 16, bit24 = 24, bit32 = 32
		};
		enum struct CompressMethod :uint32_t {
			BI_RGB = 0,
			BI_RLE8 = 1,
			BI_RLE4 = 2,
			BI_BITFIELDS = 3,
			BI_JPEG = 4,
			BI_PNG = 5,
			BI_ALPHABITFIELDS = 6,
			BI_CMYK = 11,
			BI_CMYKRLE8 = 12,
			BI_CMYKRLE4 = 13
		};
		struct Header {
			char signature[2];
			uint32_t img_size;
			uint16_t reserved1;
			uint16_t reserved2;
			uint32_t offset;
		};

		/// Windows BITMAPINFOHEADER
		struct DIB {
			uint32_t size;
			int32_t width;
			int32_t height;
			uint16_t colorplane_num;
			BitDepth bitdepth;
			CompressMethod compress_method;
			uint32_t raw_img_size;
			int32_t horizontal_ppm;
			int32_t vertical_ppm;
			uint32_t color_num;
			uint32_t important_color_num;
		};

		std::string_view signature() const
		{
			return header.signature;
		}

		uint32_t pad() const
		{
			auto d = (dib.width % 4);
			return (d == 0) ? 0 : 4 - d;
		}

		uint32_t pixel_size() const
		{
			return std::abs(dib.width * dib.height);
		}
		Header header;
		DIB dib;
	};

	std::string str_info(const Bmp::Header& head)
	{
		return std::format(
			"signature={}\n"
			"img_size={}\n"
			"offset={}\n",
			head.signature,
			head.img_size,
			head.offset);
	}

	std::string str_info(const Bmp::DIB& dib)
	{
		return std::format(
			"size={}\n"
			"width={}\n"
			"height={}\n"
			"colorplane_num={}\n"
			"bitdepth={}\n"
			"compress_method={}\n"
			"raw_img_size={}\n"
			"horizontal_ppm={}\n"
			"vertical_ppm={}\n"
			"color_num={}\n"
			"important_color_num={}\n",
			dib.size,
			dib.width,
			dib.height,
			dib.colorplane_num,
			static_cast<uint16_t>(dib.bitdepth),
			static_cast<uint32_t>(dib.compress_method),
			dib.raw_img_size,
			dib.horizontal_ppm,
			dib.vertical_ppm,
			dib.color_num,
			dib.important_color_num
		);
	}

	template <typename PX = Rgb24, size_t PX_SIZE = 3>
	struct BmpRowIt {
		using pixel_type = PX;
		using row_type = std::vector<pixel_type>;

		struct iterator
		{
			using iterator_category = std::input_iterator_tag;
			using value_type = row_type;
			using difference_type = int64_t;
			using pointer = int64_t;
			using reference = const row_type&;
			using self_type = iterator;

			iterator(pointer _ptr, BmpRowIt& _it) : ptr{ _ptr }, it{ _it } {}

			self_type& operator++()
			{
				--ptr;
				return *this;
			}
			self_type operator++(int)
			{
				self_type retval = *this;
				++(*this);
				return retval;
			}
			bool operator==(self_type other) const { return ptr == other.ptr; }
			bool operator!=(self_type other) const { return !(*this == other); }
			reference operator*() const { return it(ptr); }

		private:
			pointer ptr;
			BmpRowIt& it;
		};

		BmpRowIt(std::istream& _is, const Bmp& bmp) :
			is{ _is },
			offset{ bmp.header.offset },
			w{ static_cast<uint32_t>(std::abs(bmp.dib.width)) },
			h{ static_cast<uint32_t>(std::abs(bmp.dib.height)) },
			row_size{ w * PX_SIZE + bmp.pad() },
			row_buf{},
			px_buf{}
		{
			row_buf.reserve(w);
		}

		row_type& operator()(int64_t ro)
		{
			is.seekg(offset + row_size * (ro - 1), std::ios::beg);
			row_buf.clear();

			for (uint32_t i = 0; i < w; ++i) {
				consume_pixel(is, px_buf);
				row_buf.push_back(px_buf);
			}

			return row_buf;
		}

		iterator begin() { return iterator{ h, *this }; }
		iterator end() { return iterator{ 0, *this }; }

	private:
		std::istream& is;
		pixel_type px_buf;
		row_type row_buf;
		const uint32_t offset;
		const uint32_t w;
		const uint32_t h;
		const uint32_t row_size;
	};

	std::istream& operator>>(std::istream& is, Bmp::Header& head)
	{
		return is
			.read(&head.signature[0], 2)
			.read(reinterpret_cast<char*>(&head.img_size), 4)
			.read(reinterpret_cast<char*>(&head.reserved1), 2)
			.read(reinterpret_cast<char*>(&head.reserved1), 2)
			.read(reinterpret_cast<char*>(&head.offset), 4);
	}

	std::istream& operator>>(std::istream& is, Bmp::DIB& dib)
	{
		return is
			.read(reinterpret_cast<char*>(&dib.size), 4)
			.read(reinterpret_cast<char*>(&dib.width), 4)
			.read(reinterpret_cast<char*>(&dib.height), 4)
			.read(reinterpret_cast<char*>(&dib.colorplane_num), 2)
			.read(reinterpret_cast<char*>(&dib.bitdepth), 2)
			.read(reinterpret_cast<char*>(&dib.compress_method), 4)
			.read(reinterpret_cast<char*>(&dib.raw_img_size), 4)
			.read(reinterpret_cast<char*>(&dib.horizontal_ppm), 4)
			.read(reinterpret_cast<char*>(&dib.vertical_ppm), 4)
			.read(reinterpret_cast<char*>(&dib.color_num), 4)
			.read(reinterpret_cast<char*>(&dib.important_color_num), 4);
	}



	struct BmpReader {
		BmpReader(std::istream& input_stream, std::ostream& error_stream) :m_is{ input_stream }, m_fos{ error_stream }
		{}


		void fail(const std::string& msg) {
			m_fos << msg << '\n';
		}

		bool read_meta(Bmp& bmp)
		{
			m_is.seekg(0);
			m_is >> bmp.header;
			if (bmp.signature() != "BM") {
				fail("unknow signature");
				goto fail_return;
			}
			m_is >> bmp.dib;
			if (bmp.dib.size != 40) {
				fail("not support DIB");
				goto fail_return;
			}

			if (bmp.dib.bitdepth != Bmp::BitDepth::bit24) {
				fail("not support bitdepth");
				goto fail_return;
			}

			if (bmp.dib.compress_method != Bmp::CompressMethod::BI_RGB) {
				fail("not support compression method");
				goto fail_return;
			}

			return true;

		fail_return:
			m_is.setstate(std::ios::badbit);
			return false;
		}

		std::istream& m_is;
		std::ostream& m_fos;
	};


	struct BmpFileReader {
		BmpFileReader(const std::string& _path) :path{ _path }, ifs{ _path ,  std::ios::binary }, reader{ ifs , std::cerr }, bmp{} {};

		
		void info_to(std::ostream& os) const {
			os << str_info(bmp.header) << str_info(bmp.dib);
		}

		bool fetch_meta() {
			if (!ifs.is_open()) {
				return false;
			}
			return reader.read_meta(bmp);
		}

		auto iter() {
			return BmpRowIt{ ifs , bmp };
		}

		
		const std::string path;
		Bmp bmp;
		BmpReader reader;
		std::ifstream ifs;

	};


}