#pragma once

#include "deflate_generator.hpp"
#include "png_error.hpp"
#include "pixel.hpp"
#include <iostream>
#include <fstream>
#include <optional>


/// https://www.w3.org/TR/png/#13Decompression
namespace img::png {

	/// @brief for changing endian purpose
	template<typename T, size_t SIZE = sizeof(T), size_t MAX_IDX = SIZE - 1>
		requires std::integral<T>
	void read_reverse(std::istream& is, T& buf) {
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

	enum struct FilterType :uint8_t {
		None = 0,
		Sub ,
		Up ,
		Average ,
		Paeth,
	};

	int num_channel(ColorType c) {
		switch (c)
		{
		case ColorType::indexed:
			return 1;
		case ColorType::grayscale:
			return 1;
		case ColorType::grayscale_a:
			return 2;
		case ColorType::truecolor:
			return 3;
		case ColorType::truecolor_a:
			return 4;
		default:
			return 0;
		}
	}

	struct IHDR {
		uint32_t width;
		uint32_t height;
		BitDepth bitdetph;
		ColorType color_type;
		uint8_t compress_method;
		uint8_t filter_method;
		bool interlace;
	};

	struct Png {
		
		size_t row_size() const{
			return num_channel(ihdr.color_type) * static_cast<int>(ihdr.bitdetph) * ihdr.width / 8;
		}

		IHDR ihdr;
	};

	std::istream& operator>>(std::istream& is, IHDR& ihdr) {
		read_reverse(is, ihdr.width);
		read_reverse(is, ihdr.height);
	
		return is
			.read(reinterpret_cast<char*>(&ihdr.bitdetph), 1)
			.read(reinterpret_cast<char*>(&ihdr.color_type), 1)
			.read(reinterpret_cast<char*>(&ihdr.compress_method), 1)
			.read(reinterpret_cast<char*>(&ihdr.filter_method), 1)
			.read(reinterpret_cast<char*>(&ihdr.interlace), 1);
	}



	bool goto_chunk(std::istream& is, ChunkId id) {
		uint32_t last_size = 0;
		uint32_t last_id = 0;
		while (is.good()) {
			read_reverse(is, last_size);
			read_reverse(is, last_id);
			if (last_id == static_cast<uint32_t>(id)) {
				return true;
			}
			if (last_id == static_cast<uint32_t>(ChunkId::IEND)) {
				return false;
			}
			is.seekg(last_size + 4, std::ios::cur);//skip crc and point to next chunk
		}
		return false;
	}

	std::error_code read_meta(std::istream& is, Png& png)
	{
		uint64_t signature{};
		read_reverse(is, signature);

		std::error_code ec;
		if (signature != PNG_SIGNATURE) {
			ec = PngError::invalid_signature;
			goto fail_return;
		}

		if (!goto_chunk(is, ChunkId::IHDR)) {
			ec = PngError::invalid_ihdr;
			goto fail_return;
		}

		is >> png.ihdr; //chunk data

		is.seekg(4, std::ios::cur);//skip crc and point to next chunk

		return ec;

	fail_return:
		is.setstate(std::ios::badbit);
		return ec;
	}

	using bytes_t = std::vector<uint8_t>;

	struct Rgba32_view {
		using container_t = bytes_t;
		using value_t = Rgba32;

		struct iterator{
			using view_type = Rgba32_view;
			using iterator_category = std::input_iterator_tag;
			using value_type = value_t;
			using difference_type = std::ptrdiff_t;
			using pointer = size_t;
			using reference = value_type;
			using self_type = iterator;
			iterator(pointer _ptr, view_type& _view) :ptr{ _ptr }, view{_view} {}

			self_type& operator++()
			{
				++ptr;
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
			reference operator*() const { return view[ptr]; }
		private:
			pointer ptr;
			view_type& view;
		};
	
		Rgba32_view(container_t& c) :data{c} {}
	
		value_t operator[](size_t i) {
			auto pos = i * 4;
			return value_t{data[pos], data[pos + 1] , data[pos + 2] , data[pos + 3]};
		}

		size_t size() const {
			return data.size() / 4;
		}
		iterator begin() { return iterator{ 0, *this }; }
		iterator end() { return iterator{ size(), *this }; }
	private:
		container_t& data;
	};

	int paeth(int a, int b, int c) {
		int p = a + b - c;
		int pa = std::abs(p - a);
		int pb = std::abs(p - b);
		int pc = std::abs(p - c);

		if (pa <= pb && pa <= pc) {
			return a;
		}
		if (pb <= pc) {
			return b;
		}
		return c;
	}
	
	template<typename V = Rgba32_view>
	struct Row_decoder {
		using row_type = bytes_t;
		using view_type = V;
		

		Row_decoder(std::istream& is, const Png& png) :m_is{ is }, m_png{ png }, row_excl_filt_size{ png.row_size() } {
		
		}

		Generator<view_type*> operator()() {
			if (!goto_chunk(m_is, ChunkId::IDAT)) {
				throw std::system_error(make_error_code(PngError::idat_not_found));
			}

			deflate::Inflater_generator decomp{ m_is };
		
			auto gen = decomp();

			cur_row.reserve(row_excl_filt_size);
			prev_row.reserve(row_excl_filt_size);

			while (gen) {
				auto type = static_cast<FilterType>(gen());
				++acc_size;

				cur_row.clear();
				switch (type)
				{
				case FilterType::None:
					acc_size += filter_none(gen);
					break;
				case FilterType::Sub:
					acc_size += filter_sub(gen);
					break;
				case FilterType::Up:
					acc_size += filter_up(gen);
					break;
				case FilterType::Average:
					acc_size += filter_avg(gen);
					break;
				case FilterType::Paeth:
					acc_size += filter_paeth(gen);
					break;
				default:
					throw std::system_error(make_error_code(PngError::invalid_idat));
				}
				result.emplace(cur_row);
				co_yield &(*result);
				std::swap(cur_row, prev_row);
			}
			co_return;
		}

	private:
		size_t filter_none(Generator<uint8_t>& gen) {
			for (size_t i = 0;i < row_excl_filt_size; i++) { //no Generator validation must improve
				cur_row.push_back(gen());
			}
			return row_excl_filt_size;
		}

		size_t filter_sub(Generator<uint8_t>& gen) {
			
			for (size_t i = 0; i < row_excl_filt_size; i++) {
				cur_row.push_back(gen() + recon_a(i));
			}
			return row_excl_filt_size;
		}

		size_t filter_up(Generator<uint8_t>& gen) {
		
			for (size_t i = 0; i < row_excl_filt_size; i++) {
				cur_row.push_back(gen() + recon_b(i));
			}
			return row_excl_filt_size;
		}

		size_t filter_avg(Generator<uint8_t>& gen) {
			for (size_t i = 0; i < row_excl_filt_size; i++) {
				cur_row.push_back(gen() + ((recon_a(i) + recon_b(i)) /2));
			}

			return row_excl_filt_size;
		}

		size_t filter_paeth(Generator<uint8_t>& gen) {
			for (size_t i = 0; i < row_excl_filt_size; i++) {
				cur_row.push_back(gen() + paeth(recon_a(i), recon_b(i), recon_c(i)));
			}

			return row_excl_filt_size;
		}

		int recon_a(size_t i) {
			if (i == 0) {
				return 0;
			}
			return cur_row[i - 1];
		}
		int recon_b(size_t i) {
			if (prev_row.size() == 0) {
				return 0;
			}
			return prev_row[i];
		}
		int recon_c(size_t i) {
			if (i == 0) {
				return 0;
			}
			return prev_row[i - 1];
		}

		row_type prev_row;
		row_type cur_row;
		size_t acc_size = 0;
		const size_t row_excl_filt_size;
		std::optional<view_type> result;
		std::istream& m_is;
		const Png& m_png;
	};

	struct PngFileReader {
		PngFileReader(const std::string& _path) :path{ _path }, ifs{ _path ,  std::ios::binary }, png{} {};

		std::error_code fetch_meta() {
			if (!ifs.is_open()) {
				return PngError::fail_open_file;
			}
			auto ec = read_meta(ifs, png);
			if (ec) {
				return ec;
			}
			if (png.ihdr.bitdetph != BitDepth::bit8) {
				return PngError::bitdepth_not_support;
			}

			if (png.ihdr.color_type != ColorType::truecolor_a) {
				return PngError::color_type_not_support;
			}

			if (png.ihdr.interlace) {
				return PngError::interlace_not_support;
			}
			return {};
		}

		Row_decoder<Rgba32_view> decoder() {
			return Row_decoder{ ifs, png };
		}
		
		const std::string path;
		Png png;
		std::ifstream ifs;

	};
}