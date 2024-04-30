#pragma once

#include "generator.hpp"
#include "deflate_error.hpp"
#include "deflate.hpp"

namespace img::deflate
{
	struct Inflater_generator
	{
		using generator_type = Generator<uint8_t>;
		Inflater_generator(std::istream& is) : m_is{ is } {}

		generator_type operator()() {

			auto header = read_head(m_is);

			if (header.CF != 8
				|| header.CINFO > 7
				|| header.FDICT != 0) {
				throw std::system_error(make_error_code(DeflateError::general_error));
			}
			window_t window{ deflate_window_size(header.CINFO) };
			while (m_is.good()) {
				bool bfinal = m_is.read_bits(1);
				BlockType btype = static_cast<BlockType>(m_is.read_bits(2));
				switch (btype) {
					case BlockType::reserved:
					case BlockType::no_compress:
					case BlockType::fixed:
						throw std::system_error(make_error_code(DeflateError::general_error));
					case BlockType::dynamic: {
						auto result = read_lz77(m_is);
						if (result.first) {
							throw std::system_error(make_error_code(static_cast<DeflateError>(result.first)));
						}
						auto gen = decode_lz77(*result.second, window);
						while (gen)
						{
							co_yield gen();
						}
					}
				}
				if (bfinal)
				{
					co_return;
				}
			}

			throw std::system_error(make_error_code(DeflateError::general_error));
		}

	private:
		generator_type decode_lz77(const Lz77code& lz, window_t& window) {
			int32_t symbol;         /* decoded symbol */
			uint32_t len;            /* length for copy */
			uint32_t dist;      /* distance for copy */
			size_t outcnt = 0;

			do {
				symbol = m_is.read_code(lz.lencode);
				if (symbol < 0) {
					throw std::system_error(make_error_code(DeflateError::general_error));
				}
				if (symbol < 256) {
					window[outcnt] = symbol;
					co_yield window[outcnt];
					outcnt++;
				}
				else if (symbol > 256) {
					symbol -= 257;

					if (symbol >= 29) { 
						
						throw std::system_error(make_error_code(DeflateError::invalid_huffman_code));
					
					} //invalid fixed code

					len = lens[symbol] + m_is.read_bits(lext[symbol]);
					symbol = m_is.read_code(lz.distcode);

					if (symbol < 0) { 
						throw std::system_error(make_error_code(DeflateError::general_error)); 
					}     /* invalid symbol */
					dist = dists[symbol] + m_is.read_bits(dext[symbol]);

					if (dist > outcnt || dist > window.size()) {
						throw std::system_error(make_error_code(DeflateError::distance_exceeded));
					}
				

					while (len--) {

						window[outcnt] = window[outcnt - dist];
						co_yield window[outcnt];
						outcnt++;
					}
				}
			} while (symbol != 256);

			co_return;
		}
		InflateStream m_is;
	};
}