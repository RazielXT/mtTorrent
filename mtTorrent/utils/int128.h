#pragma once

#include <cstdint>

#if defined (_M_X64) || defined (__x86_64__)
#define HAS_INT128

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#if defined(__SIZEOF_INT128__)

#if !(defined(__clang__) && defined(_MSC_VER))
#define HAS_INT128_DIV
#endif

using int128_t = __int128_t;
using uint128_t = __uint128_t;

constexpr uint64_t lo_word(const uint128_t val) noexcept
{
	return static_cast<uint64_t>(val);
}

constexpr uint64_t hi_word(const uint128_t val) noexcept
{
	return static_cast<uint64_t>(val >> 64);
}

constexpr uint128_t mul_words(const uint64_t a, const uint64_t b) noexcept
{
	return (uint128_t)a * (uint128_t)b;
}

#ifdef HAS_INT128_DIV
constexpr uint128_t div_rem(const uint128_t dividend, const uint64_t divisor, uint128_t& remainder) noexcept
{
	remainder = dividend % divisor;
	return dividend / divisor;
}
#else
static inline uint64_t _udiv128(uint64_t highDividend, uint64_t lowDividend, uint64_t divisor,
	uint64_t* remainder)
{
	uint64_t result;
	__asm__("divq %[d]"
		: "=a"(result), "=d"(*remainder)
		: [d] "r"(divisor), "a"(lowDividend), "d"(highDividend));
	return result;
}

static inline uint128_t div_rem(uint128_t dividend, const uint64_t divisor, uint128_t& remainder) noexcept
{
	uint128_t tmp = ((uint128_t)divisor) << 64;
	uint64_t loQ, tmpR;
	uint64_t hiQ = 0ULL;
	//check overflow
	if (tmp <= dividend)
	{
		dividend -= tmp;
		hiQ = 1;
	}
	loQ = _udiv128((uint64_t)(dividend >> 64), (uint64_t)dividend, divisor, &tmpR);
	remainder = tmpR;
	return (((uint128_t)hiQ) << 64) | loQ;
}
#endif

constexpr uint128_t make_dword(const uint64_t lo, const uint64_t hi) noexcept
{
	return (uint128_t)lo | ((uint128_t)hi << 64);
}

constexpr uint128_t shiftl_half_word(uint128_t val) noexcept
{
	return val << 64;
}

inline void shiftr_half_word(uint128_t& val) noexcept
{
	val >>= 64;
}

inline void shiftr_half_word(int128_t& val) noexcept
{
	val >>= 64;
}

#else
struct uint128_t
{
	uint64_t lo{ 0 };
	uint64_t hi{ 0 };

	uint128_t() = default;

	explicit constexpr uint128_t(const uint64_t _lo, const uint64_t _hi) noexcept
		: lo(_lo)
		, hi(_hi)
	{
	}

	constexpr uint128_t(const uint64_t val) noexcept
		: lo(val)
		, hi(0)
	{
	}

	constexpr bool operator==(const uint64_t val) const noexcept
	{
		return hi == 0 && lo == val;
	}

	inline uint128_t operator<<=(const unsigned char amount) noexcept
	{
		hi = __shiftleft128(lo, hi, amount);
		lo <<= amount;
		return *this;
	}

	uint128_t& operator>>=(const unsigned char amount) noexcept
	{
		lo = __shiftright128(lo, hi, amount);
		hi >>= amount;
		return *this;
	}

	inline void operator+=(const uint64_t val) noexcept
	{
		hi += _addcarry_u64(0, lo, val, &lo);
	}

	inline void operator+=(const uint128_t val) noexcept
	{
		hi += _addcarry_u64(0, lo, val.lo, &lo);
		hi += val.hi;
	}

	inline void operator-=(const uint64_t val) noexcept
	{
		hi -= _subborrow_u64(0, lo, val, &lo);
	}

	inline void operator-=(const uint128_t val) noexcept
	{
		hi -= _subborrow_u64(0, lo, val.lo, &lo);
		hi -= val.hi;
	}

	constexpr void operator|=(const uint128_t val) noexcept
	{
		lo |= val.lo;
		hi |= val.hi;
	}

	inline void operator--(int) noexcept
	{
		*this -= 1ULL;
	}
};

inline uint128_t operator+(uint128_t lhs, uint128_t rhs) noexcept
{
	lhs += rhs;
	return lhs;
}

inline uint128_t operator-(uint128_t lhs, uint128_t rhs) noexcept
{
	lhs -= rhs;
	return lhs;
}

inline uint128_t operator<<(uint128_t lhs, const unsigned char amount) noexcept
{
	lhs <<= amount;
	return lhs;
}

inline uint128_t operator>>(uint128_t lhs, const unsigned char amount) noexcept
{
	lhs >>= amount;
	return lhs;
}

inline uint128_t mul_words(const uint64_t a, const uint64_t b) noexcept
{
	uint128_t r;
	r.lo = _umul128(a, b, &r.hi);
	return r;
}

constexpr bool operator<(const uint128_t lhs, const uint128_t rhs) noexcept
{
	return (lhs.hi == rhs.hi) ? (lhs.lo < rhs.lo) : (lhs.hi < rhs.hi);
}

constexpr bool operator>(const uint128_t lhs, const uint128_t rhs) noexcept { return rhs < lhs; }
constexpr bool operator<=(const uint128_t lhs, const uint128_t rhs) noexcept { return !(rhs < lhs); }
constexpr bool operator>=(const uint128_t lhs, const uint128_t rhs) noexcept { return !(lhs < rhs); }
constexpr uint64_t lo_word(const uint128_t x) noexcept { return x.lo; }
constexpr uint64_t hi_word(const uint128_t x) noexcept { return x.hi; }
constexpr uint128_t make_dword(const uint64_t lo, const uint64_t hi) noexcept { return uint128_t{ lo, hi }; }

inline uint128_t div_rem(uint128_t dividend, const uint64_t divisor, uint128_t& remainder) noexcept
{
	const auto tmp = make_dword(0ULL, divisor);//divisor * base
	uint64_t hiQ = 0ULL;
	//check overflow
	if (tmp <= dividend)
	{
		dividend -= tmp;
		hiQ = 1ULL;
	}
	return make_dword(_udiv128(dividend.hi, dividend.lo, divisor, &remainder.lo), hiQ);
}

constexpr uint128_t shiftl_half_word(const uint128_t val) noexcept
{
	return make_dword(0ULL, val.lo);
}

inline void shiftr_half_word(uint128_t& val) noexcept
{
	val.lo = val.hi;
	val.hi = 0;
}

struct int128_t : uint128_t
{
	constexpr int128_t(uint64_t src)
		: uint128_t(src)
	{
	}
	constexpr int128_t(const uint128_t& src)
		: uint128_t(src)
	{
	}
};

constexpr void shiftr_half_word(int128_t& val) noexcept
{
	val.lo = val.hi;
	val.hi = static_cast<int64_t>(val.hi) >> 63; //preserve sign
}

inline int128_t operator-(int128_t lhs, int128_t rhs) noexcept
{
	lhs -= rhs;
	return lhs;
}

constexpr bool operator<(const int128_t lhs, const int128_t rhs) noexcept
{
	return (lhs.hi == rhs.hi) ? (lhs.hi > 0 ? lhs.lo < rhs.lo : lhs.lo > rhs.lo) : ((int64_t)lhs.hi < (int64_t)rhs.hi);
}
constexpr bool operator>(const int128_t lhs, const int128_t rhs) noexcept { return rhs < lhs; }
constexpr bool operator<=(const int128_t lhs, const int128_t rhs) noexcept { return !(rhs < lhs); }
constexpr bool operator>=(const int128_t lhs, const int128_t rhs) noexcept { return !(lhs < rhs); }
constexpr uint64_t lo_word(const int128_t x) noexcept { return x.lo; }
constexpr uint64_t hi_word(const int128_t x) noexcept { return x.hi; }

#endif
#else
constexpr uint64_t div_rem(const uint64_t dividend, const uint32_t divisor, uint64_t& remainder) noexcept
{
	remainder = dividend % divisor;
	return dividend / divisor;
}

constexpr uint64_t mul_words(const uint32_t a, const uint32_t b) noexcept
{
	return(uint64_t)a * (uint64_t)b;
}

constexpr uint32_t lo_word(const uint64_t val) noexcept
{
	return static_cast<uint32_t>(val);
}

constexpr uint32_t hi_word(const uint64_t val) noexcept
{
	return static_cast<uint32_t>(val >> 32);
}

constexpr uint64_t make_dword(const uint32_t lo, const uint32_t hi) noexcept
{
	return (uint64_t)lo | ((uint64_t)hi << 32);
}

constexpr uint64_t shiftl_half_word(const uint64_t val) noexcept
{
	return val << 32;
}

inline void shiftr_half_word(uint64_t& val) noexcept
{
	val >>= 32;
}

inline void shiftr_half_word(int64_t& val) noexcept
{
	val >>= 32;
}
#endif