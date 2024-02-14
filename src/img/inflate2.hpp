#pragma once

#include <iostream>
#include <format>
#include <vector>

namespace img {
	constexpr auto MAXBITS = 15;
	constexpr auto MAXLCODES = 286;
	constexpr auto MAXDCODES = 30;
	constexpr auto MAXCODES = MAXLCODES + MAXDCODES;
	constexpr auto FIXLCODES = 288;
	struct DeflateDecompress
	{
		struct Huffman {
			constexpr Huffman(size_t ncnt, size_t nsym):count(ncnt, 0), symbol(nsym, 0)
			{
			}
			int build(std::vector<uint16_t>& length, int n) {
				for (int symbol = 0; symbol < n; symbol++) {
					count[length[symbol]]++;
				}
				if (count[0] == n) {
					return 0;
				}
				return 1;
			}

			std::vector<uint16_t> count;
			std::vector<uint16_t> symbol;
		};
		union CMF {
			uint8_t data;
			struct {
				uint8_t CF : 4;//compress method deflate=8
				uint8_t CINFO : 4;//window size 0-7
			};
		};
		union FLG {
			uint8_t data;
			struct {
				uint8_t FCHECK : 5;
				uint8_t FDICT : 1;
				uint8_t FLEVEL : 2;
			};
		};
		enum struct BlockType :uint8_t {
			no_compression,
			fixed_huffman,
			dynamic_huffman,
			reserved
		};

		DeflateDecompress(std::istream& is): m_is {is}{}

		int decompress() {
			CMF cmf{};
			read_to(cmf.data);
			if (cmf.CF != 0x8 //must be deflate
				|| cmf.CINFO != 0x7 //must be 7: 32768 window size
				) {
				return 1;
			}
			FLG flg{};
			read_to(flg.data);
			if (flg.FDICT != 0) {// must no FDICT
				return 1;
			}
			
			while (m_is.good()) {
			
				auto bfinal = read_bits(1);
				BlockType btype = static_cast<BlockType>(read_bits(2));

				if (btype == BlockType::reserved) {
					return 1;
				}

				if (btype == BlockType::no_compression) {
					return 1;//not support yet
				}
				else {
					if (btype == BlockType::dynamic_huffman) {
						
						constexpr size_t lengths_array_size = 320;
						constexpr int max_code_length_codewords = 19;
						static const uint8_t code_length_code_order[max_code_length_codewords] = {
							16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
						};

						uint8_t lengths[lengths_array_size]{};
						//std::vector<uint8_t> lengths(lengths_array_size);
						int literal_length_codes = read_bits(5) + 257;
						int distance_codes = read_bits(5) + 1;
						int code_length_codes = read_bits(4) + 4;

						std::cout << std::format("literal_length_codes={} distance_codes={} code_length_codes={}\n", literal_length_codes, distance_codes, code_length_codes);
					}
				}

				std::cout << std::format("BFINAL={} BTYPE={}\n", bfinal, static_cast<uint8_t>(btype));
				if (bfinal) 
				{
					return 0;
				}
			} 

			return 1;
		}

		int dynamic_huffman() {
			static const uint8_t order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
			Huffman lencode{ MAXBITS + 1 , MAXLCODES };
			Huffman distcode{ MAXBITS + 1 , MAXDCODES };

			int nlen = read_bits(5) + 257;
			int ndist = read_bits(5) + 1;
			int ncode = read_bits(4) + 4;

			if (nlen > MAXLCODES || ndist > MAXDCODES) {
				return 1;
			}
			std::vector<uint16_t> lengths(MAXCODES, 0);  /* descriptor code lengths */
			
			for (int index = 0; index < ncode; index++) {
				lengths[order[index]] = read_bits(3);
			}

			auto err = lencode.build(lengths, 19);

			if (err) {
				return 1;
			}

			return 0;
		}

	
		/// @brief get bit in reverse order
		/// @param n must not over 8
		uint8_t read_bits(int n) {
			
			if (bit_avail < n) {
				read_to(byte_buf); // read unformated 1 byte 
				bits_buf = (byte_buf << bit_avail) | bits_buf;
				bit_avail += 8;
			}

			uint8_t ret = 0;
			for (int i = 0; i < n;++i) {
				ret = ((bits_buf & 1) << i) | ret;
				bits_buf >>= 1;
				--bit_avail;
			}
			return ret;
		}

		template<typename T, size_t N = sizeof(T)>
		void read_to(T& b) {
			m_is.read(reinterpret_cast<char*>(&b), N);
		}

	private:
		uint8_t byte_buf;
		uint16_t bits_buf = 0;
		uint8_t bit_avail = 0;
		std::istream& m_is;
	};
}