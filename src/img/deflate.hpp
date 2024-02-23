#pragma once

#include "generator.hpp"
#include <iostream>
#include <vector>
#include <memory>

// most code come from https://github.com/madler/zlib/blob/master/contrib/puff/puff.c
namespace img::deflate
{
	constexpr int MAXBITS = 15;
	constexpr int MAXLCODES = 286;
	constexpr int MAXDCODES = 30;
	constexpr int MAXCODES = MAXLCODES + MAXDCODES;
	constexpr int FIXLCODES = 288;

	constexpr int16_t lens[29] = { /* Size base for length codes 257..285 */
		   3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
		   35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
	constexpr int16_t lext[29] = { /* Extra bits for length codes 257..285 */
		0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
		3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
	constexpr int16_t dists[30] = { /* Offset base for distance codes 0..29 */
		1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
		257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
		8193, 12289, 16385, 24577 };
	constexpr int16_t dext[30] = { /* Extra bits for distance codes 0..29 */
		0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
		7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
		12, 12, 13, 13 };

	constexpr uint16_t order[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };//lenght code order

	constexpr size_t max_distance = 32768;

	enum struct BlockType :uint8_t {
		no_compress,
		fixed,
		dynamic,
		reserved
	};


	template<typename T, size_t SIZE>
	struct CircularBuffer {

		T& operator[](std::ptrdiff_t index) {
			return data[index % SIZE];
		}

		constexpr size_t size() const {
			return SIZE;
		}
	private:
		T data[SIZE];
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

		std::vector<int16_t> count;
		std::vector<int16_t> symbol;
	};

	struct LZ77code {
		LZ77code(size_t nlen, size_t ndist) :
			lencode{ MAXBITS + 1 , nlen }, 
			distcode{ MAXBITS + 1 , ndist } {}
		Huffman lencode;
		Huffman distcode;
	};

	struct InflateStream
	{
		InflateStream(std::istream& is) :m_is{ is } {}

		int read_code(const Huffman& h) {
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

		uint32_t read_bits(int n) {
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

		bool good() const {
			return m_is.good();
		}
	private:
		uint8_t byte_buf = 0;
		uint32_t bits_buf = 0;
		uint8_t bit_avail = 0;
		std::istream& m_is;
	};


	int LZ77Decode(InflateStream& is, const LZ77code& lz) {

		int32_t symbol;         /* decoded symbol */
		uint32_t len;            /* length for copy */
		uint32_t dist;      /* distance for copy */
		CircularBuffer<uint8_t, max_distance> dict_buf;
		size_t outcnt = 0;

		do {
			symbol = is.read_code(lz.lencode);
			if (symbol < 0) {
				return symbol;
			}
			if (symbol < 256) {
				dict_buf[outcnt] = symbol;
				outcnt++;
			}
			else if (symbol > 256) {
				symbol -= 257;

				if (symbol >= 29) { return -9; }; //invalid fixed code

				len = lens[symbol] + is.read_bits(lext[symbol]);
				symbol = is.read_code(lz.distcode);

				if (symbol < 0) { return symbol; }     /* invalid symbol */
				dist = dists[symbol] + is.read_bits(dext[symbol]);

				if (dist > outcnt) {
					return -11;
				}
				if (dist > dict_buf.size()) {
					return -12;
				}

				while (len--) {

					dict_buf[outcnt] = dict_buf[outcnt - dist];
					outcnt++;
				}
			}
		} while (symbol != 256);

		std::cout << "usize=" << outcnt << "\n";
		return 0;
	}

	/// read deflate header
	Header read_head(InflateStream& is) {
		Header head;
		is.read_to(head.data);
		return head;
	}

	using LZ77code_ptr = std::unique_ptr<LZ77code>;
	using LZ77_read_result = std::pair<int, LZ77code_ptr>; // error code, table


	/// dynamic huffman
	LZ77_read_result read_LZ77(InflateStream& is) {
		
		auto lz = std::make_unique<LZ77code>(MAXLCODES, MAXDCODES);

		int nlen = is.read_bits(5) + 257;
		int ndist = is.read_bits(5) + 1;
		int ncode = is.read_bits(4) + 4;

		if (nlen > MAXLCODES || ndist > MAXDCODES) {
			return {-3, nullptr};
		}

		std::vector<int16_t> lengths(MAXCODES, 0); /* descriptor code lengths */

		int index;
		for (index = 0; index < ncode; index++) {
			lengths[order[index]] = is.read_bits(3);
		}

		int err = lz->lencode.build(lengths.data(), 19);
		
		if (err) {
			return { -4, nullptr };
		}

		index = 0;
		int codelen = nlen + ndist;
		while (index < codelen) {

			int sym = is.read_code(lz->lencode);
			if (sym < 0) {
				return { -99, nullptr };
			}
			if (sym < 16) {
				lengths[index++] = sym;
			}
			else {
				int len = 0;

				if (sym == 16) {         /* repeat last length 3..6 times */
					if (index == 0) { return {}; }      /* no last length! */
					len = lengths[index - 1];       /* last length */
					sym = 3 + is.read_bits(2);
				}
				else if (sym == 17) {     /* repeat zero 3..10 times */
					sym = 3 + is.read_bits(3);
				}
				else {                    /* == 18, repeat zero 11..138 times */
					sym = 11 + is.read_bits(7);
				}
				if (index + sym > codelen) {
					return { -6, nullptr };
				}
				while (sym--)            /* repeat last or zero symbol times */
					lengths[index++] = len;
			}

		}

		if (lengths[256] == 0)
			 return { -9, nullptr };

		err = lz->lencode.build(lengths.data(), nlen);

		if (err && (err < 0 || nlen != lz->lencode.count[0] + lz->lencode.count[1])) {
			 return { -7, nullptr };
		}

		err = lz->distcode.build(lengths.data() + nlen, ndist);

		if (err && (err < 0 || ndist != lz->distcode.count[0] + lz->distcode.count[1]))
			return { -8, nullptr };      /* only allow incomplete codes if just one code */

		return { 0, std::move(lz) };
	}

	int inflate(InflateStream& is) {
		int err = 0;

		while (is.good()) {
			bool bfinal = is.read_bits(1);
			BlockType btype = static_cast<BlockType>(is.read_bits(2));
			switch (btype) {
			case BlockType::reserved:
				return -20;
			case BlockType::no_compress:
				return -20;
			case BlockType::fixed:
				return -20;
			case BlockType::dynamic: {
				auto result = read_LZ77(is);
				if (result.first) {
					return result.first;
				}
				auto ec = LZ77Decode(is, *result.second);
				if (ec) {
					return ec;
				}
				break;
			}
			}
			if (bfinal)
			{
				return 0;
			}
		}

		return -20;
	}
}