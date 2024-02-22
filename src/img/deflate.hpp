#pragma once

#include <iostream>
#include <vector>

// most code come from https://github.com/madler/zlib/blob/master/contrib/puff/puff.c
namespace img::deflate
{
	


	constexpr int MAXBITS = 15;
	constexpr int MAXLCODES = 286;
	constexpr int MAXDCODES = 30;
	constexpr int MAXCODES = MAXLCODES + MAXDCODES;
	constexpr int FIXLCODES = 288;

	struct Huffman {
		constexpr Huffman(size_t ncnt, size_t nsym) :count(ncnt, 0), symbol(nsym, 0)
		{
		}
		int build(int16_t* length, int n) {
			for (int len = 0; len <= MAXBITS; len++) {
				count[len] = 0;
			}
			for (int sym = 0; sym < n; sym++) {
				count[length[sym]]++;
			}
			if (count[0] == n) {
				return 0;
			}
			int left = 1;
			for (int len = 1; len <= MAXBITS; ++len) {
				left <<= 1;
				left -= count[len];
				if (left < 0) {
					return left;
				}
			}
			int16_t off[MAXBITS + 1]{};
			for (int len = 1; len < MAXBITS; len++) {
				off[len + 1] = off[len] + count[len];
			}
			for (int sym = 0; sym < n; sym++) {
				if (length[sym] != 0) {
					symbol[off[length[sym]]++] = sym;
				}
			}
			return left;
		}

		/*void info_count() const {
			std::cout << "count=" << count.size() << '\n';
			int x = 0;
			for (auto i : count) {

				std::cout << "#" << x << "=" << i << " -> " << std::bitset<16>(i) << "\n";
				x++;
			}
		}
		void info_symbol() const {
			std::cout << "symbol=" << symbol.size() << '\n';
			int x = 0;
			for (auto i : symbol) {
				std::cout << "#" << x << "=" << i << " -> " << std::bitset<16>(i) << "\n";
				x++;
			}
		}
		void info() const
		{
			info_count();
			info_symbol();
		}*/

		std::vector<int16_t> count;
		std::vector<int16_t> symbol;
	};

	union Header {
		uint16_t data;
		struct {
			uint8_t CF : 4;
			uint8_t CINFO : 4;
			uint8_t FCHECK : 5;
			uint8_t FDICT : 1;
			uint8_t FLEVEL : 2;
		};
	};

	enum struct BlockType :uint8_t {
		no_compress,
		fixed,
		dynamic,
		reserved
	};

	struct Inflater
	{
		using window_t = std::vector<uint8_t>;
	
		Inflater(std::istream& is, window_t& w) : m_is{ is }, m_window{w} {
		
		}

		int consume_head(Header& head) {
			read_to(head.data);
			return 0;
		}

		int decompress(){
			int err = 0;
			while (m_is.good()) {

				auto bfinal = read_bits(1);
				BlockType btype = static_cast<BlockType>(read_bits(2));

				switch (btype) {
				case BlockType::reserved:
					return 10;
				case BlockType::no_compress:
					return 10;
				case BlockType::fixed:
					return 10;
				case BlockType::dynamic:
					err = dynamic_huffman();
					break;
				}
				if (err) {
					return err;
				}

				if (bfinal)
				{
					return 0;
				}
			}

			return 10;
		}


	private:

		int dynamic_huffman() {
			static constexpr uint16_t order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
			Huffman lencode{ MAXBITS + 1 , MAXLCODES };
			Huffman distcode{ MAXBITS + 1 , MAXDCODES };

			int nlen = read_bits(5) + 257;
			int ndist = read_bits(5) + 1;
			int ncode = read_bits(4) + 4;

		
			if (nlen > MAXLCODES || ndist > MAXDCODES) {
				return -3;
			}

			std::vector<int16_t> lengths(MAXCODES, 0); /* descriptor code lengths */

			int index;
			for (index = 0; index < ncode; index++) {
				lengths[order[index]] = read_bits(3);
			}

			auto err = lencode.build(lengths.data(), 19);
	
			if (err) {
				return -4;
			}

			index = 0;
			auto codelen = nlen + ndist;
			while (index < codelen) {

				int sym = decode(lencode);
				if (sym < 0) {
					return sym;
				}
				if (sym < 16) {
					lengths[index++] = sym;
				}
				else {
					int len = 0;

					if (sym == 16) {         /* repeat last length 3..6 times */
						if (index == 0) return -5;      /* no last length! */
						len = lengths[index - 1];       /* last length */
						sym = 3 + read_bits(2);
					}
					else if (sym == 17) {     /* repeat zero 3..10 times */
						sym = 3 + read_bits(3);
					}
					else {                    /* == 18, repeat zero 11..138 times */
						sym = 11 + read_bits(7);
					}
					if (index + sym > codelen) {
						return -6;
					}
					while (sym--)            /* repeat last or zero symbol times */
						lengths[index++] = len;
				}

			}

			if (lengths[256] == 0)
				return -9;

			err = lencode.build(lengths.data(), nlen);

			if (err && (err < 0 || nlen != lencode.count[0] + lencode.count[1])) {
				return -7;
			}

			err = distcode.build(lengths.data() + nlen, ndist);

			if (err && (err < 0 || ndist != distcode.count[0] + distcode.count[1]))
				return -8;      /* only allow incomplete codes if just one code */

			return codes(lencode, distcode);
		}

		int codes(const Huffman& lencode, const Huffman& distcode) {

			static constexpr int16_t lens[29] = { /* Size base for length codes 257..285 */
			   3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
			   35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
			static constexpr int16_t lext[29] = { /* Extra bits for length codes 257..285 */
				0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
				3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
			static constexpr int16_t dists[30] = { /* Offset base for distance codes 0..29 */
				1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
				257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
				8193, 12289, 16385, 24577 };
			static constexpr int16_t dext[30] = { /* Extra bits for distance codes 0..29 */
				0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
				7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
				12, 12, 13, 13 };

			int32_t symbol;         /* decoded symbol */
			uint32_t len;            /* length for copy */
			uint32_t dist;      /* distance for copy */
			

			auto it = std::back_inserter(m_window);
			do {
				symbol = decode(lencode);
				if (symbol < 0) {
					return symbol;
				}
				if (symbol < 256) {	
					*it++ = symbol;
					
				}
				else if (symbol > 256) {
					symbol -= 257;

					if (symbol >= 29) { return -9; }; //invalid fixed code

					len = lens[symbol] + read_bits(lext[symbol]);
					symbol = decode(distcode);

					if (symbol < 0) { return symbol; }     /* invalid symbol */
					dist = dists[symbol] + read_bits(dext[symbol]);

					if (dist > m_window.size()) {
						return -10;
					}
					
					while (len--) {
						*it++ = *(m_window.end() - dist);
					}
				}
			} while (symbol != 256);

			return 0;
		}

		int decode(const Huffman& h) {
			int code = 0, first = 0, index = 0;

			for (int len = 1; len <= MAXBITS; ++len) {
				code |= read_bits(1);
				int count = h.count[len];
				if (code - count < first) {
					return h.symbol[index + (code - first)];
				}
				index += count;
				first += count;
				first <<= 1;
				code <<= 1;
			}

			return -10;
		}

		int32_t read_bits(int n) {
			while (bit_avail < n) {
				read_to(byte_buf); // read unformated 1 byte 
				bits_buf |= (byte_buf << bit_avail);
				bit_avail += 8;
			}

			uint32_t ret = bits_buf & ~(-1u << n);// -1u = 0xffff... = 0b1111...

			bits_buf >>= n;
			bit_avail -= n;

			return ret;
		}

		template<typename T, size_t N = sizeof(T)>
		void read_to(T& b) {
			m_is.read(reinterpret_cast<char*>(&b), N);
		}

	/*	void info_len(int16_t* len, int n) const {
			std::cout << "lengths\n";
			for (int index = 0; index < n; index++) {
				std::cout << "#" << index << '=' << len[index] << " -> " << std::bitset<16>(len[index]) << "\n";
			}
		};*/

		uint8_t byte_buf = 0;
		uint32_t bits_buf = 0;
		uint8_t bit_avail = 0;
		std::istream& m_is;
		window_t& m_window;
	};
}