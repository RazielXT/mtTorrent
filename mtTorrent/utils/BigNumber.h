#pragma once

#include <cstdint>
#include <vector>
#include "int128.h"

class BigNumber
{
#ifdef HAS_INT128
	using double_digit_t = uint128_t;
	using s_double_digit_t = int128_t;
	using digit_t = uint64_t;
	constexpr static uint32_t digit_bits = static_cast<uint32_t>(sizeof(digit_t) * 8);
	constexpr static double_digit_t digit_base = make_dword(0ULL, 1ULL);
#else
	using double_digit_t = uint64_t;
	using s_double_digit_t = int64_t;
	using digit_t = uint32_t;
	constexpr static uint32_t digit_bits = static_cast<uint32_t>(sizeof(digit_t) * 8);
	constexpr static double_digit_t digit_base = static_cast<double_digit_t>(1) << (digit_bits);
#endif

public:

	BigNumber() = default;
	BigNumber(const uint8_t* src, const std::size_t size);
	BigNumber(const uint32_t value, const std::size_t size);

	void Set(const uint8_t* data, std::size_t size);
	void Export(uint8_t* out, std::size_t size);

	uint8_t* GetDataBuffer();

	void SetByteSize(const std::size_t newLength);
	std::size_t GetByteSize() const;

	void Set(const uint32_t a);
	void SetZero();

	bool IsEqual(const BigNumber& other) const;
	int Compare(const BigNumber& right) const;
	bool operator==(const BigNumber& other) const;

	//return overflow if there is any
	static digit_t Add(BigNumber& a, const BigNumber& b);

	//return underflow if there is any
	static digit_t Sub(BigNumber& a, const BigNumber& b);

	//returns the result and the remainder
	static bool Div(BigNumber& result, BigNumber& remainder, const BigNumber& a, const BigNumber& b);

	//if overflows result contains least significant bits
	static bool Mul(BigNumber& result, const BigNumber& a1, const BigNumber& a2);

	//(a^ e) mod m
	static bool Powm(BigNumber& result, const BigNumber& a, const BigNumber& e, const BigNumber& m);

	//(a1 * a2) mod m
	static bool Mulm(BigNumber& result, const BigNumber& a1, const BigNumber& a2, const BigNumber& m);

private:

	void AllocateData(const std::size_t lengthToAllocate);
	std::size_t BytesToAllocationUnit(std::size_t aNumberOfBytes) const;
	std::size_t Length() const;
	void SetLength(const std::size_t newLength);

	const digit_t* GetData() const;
	digit_t* GetData();

	static digit_t Add_h(digit_t* a, const digit_t* b, std::size_t len);
	static digit_t Sub_h(digit_t* a, const digit_t* b, std::size_t len);
	static void Square_h(const digit_t* a, std::size_t len, digit_t* dst);
	static void Mul_h(const digit_t* a, std::size_t len1, const digit_t* b, std::size_t len2, digit_t* dst);

	//calculates the most significant bit(MSB) position of unsigned integer
	static uint32_t MsbPos(uint32_t v);

	//calculates the most significant bit(MSB) position of unsigned long integer
	static uint32_t MsbPos(uint64_t v);

	//Returns the product of two values
	//It is possible to multiply just values of the same length
	//The result has the double length as multiplied numbers
	static void dbi_mul(BigNumber& dblResult, const BigNumber& a1, const BigNumber& a2);

	//Divides one value by another, returns the result and the remainder
	static bool dbi_div(BigNumber& result, BigNumber& remainder, const BigNumber& dblA, const BigNumber& b);

	static void dbi2bi(BigNumber& dest, const BigNumber& src);
	static void bi2dbi(BigNumber& dest, const BigNumber& src);

	/** Represents an arbitrarily large signed integer.
	 *  Integer is stored in memory in a little format.
	 *  It means the least significant bit of the number is at the beginning of the m_data.
	 *  Number is allocated with granularity (unit) of sizeof(data_t)
	 */
	std::vector<digit_t> m_data;

};
