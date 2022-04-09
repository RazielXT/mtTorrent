#include "BigNumber.h"
#include <cstdlib>
#include <algorithm>

BigNumber::BigNumber(const uint8_t* src, const size_t size)
{
	Set(src, size);
}

BigNumber::BigNumber(const uint32_t value, const size_t size)
{
	SetByteSize(size);
	Set(value);
}

BigNumber::BigNumber(const BigNumber& other)
{
	m_data = other.m_data;
}

void BigNumber::Export(uint8_t* out, size_t size)
{
	int i = ((int)std::min(GetByteSize(), size)) - 1;
	auto dataPtr = (const uint8_t*)GetData();

	while (i >= 0)
	{
		*out = dataPtr[i];
		i--;
		out++;
	}
}

uint8_t* BigNumber::GetDataBuffer()
{
	return (uint8_t*) m_data.data();
}

const BigNumber::digit_t* BigNumber::GetData() const
{
	return m_data.data();
}

BigNumber::digit_t* BigNumber::GetData()
{
	return m_data.data();
}

void BigNumber::SetZero()
{
	if (!m_data.empty())
	{
		memset(m_data.data(), 0, GetByteSize());
	}
}

size_t BigNumber::GetByteSize() const
{
	return Length() * sizeof(digit_t);
}

size_t BigNumber::BytesToAllocationUnit(size_t aNumberOfBytes) const
{
	return (aNumberOfBytes + sizeof(digit_t) - 1) / sizeof(digit_t);
}

void BigNumber::AllocateData(const size_t lengthToAllocate)
{
	m_data.resize(lengthToAllocate);
}

void BigNumber::SetLength(const size_t newLength)
{
	if (Length() != newLength)
	{
		return AllocateData(newLength);
	}
}

size_t BigNumber::Length() const
{
	return m_data.size();
}

void BigNumber::SetByteSize(const size_t newLength)
{
	SetLength(BytesToAllocationUnit(newLength));
}

void BigNumber::Set(const uint8_t* data, size_t size)
{
	size_t currentSize = GetByteSize();
	if (currentSize < size)
	{
		const auto toAllocate = BytesToAllocationUnit(size);
		AllocateData(toAllocate);
		currentSize = GetByteSize();
	}

	auto* q = (uint8_t*)m_data.data();
	const uint8_t* p = data + size - 1;

	for (size_t i = 0; i < size; i++)
	{
		*(q++) = *(p--);
	}

	const auto toerase = currentSize - size;
	if (toerase)
	{
		memset((char*)m_data.data() + size, 0, toerase);
	}
}

bool BigNumber::Powm(BigNumber& result, const BigNumber& a, const BigNumber& e, const BigNumber& m)
{
	if (a.Length() != e.Length() || e.Length() != m.Length())
	{
		return false;
	}
	result.SetLength(m.Length());
	result.Set(1);

	BigNumber na2(a);

	auto cnt = (int)e.Length() - 1;
	for (; cnt >= 0; cnt--)
	{
		if (e.m_data[cnt])
		{
			break;
		}
	}

	if (cnt < 0)
	{
		return true;	// Todo: test this case
	}


	if (!e.m_data[cnt])
	{
		return true; //0
	}

	for (size_t i = 0; i <= cnt; i++)
	{
		auto digit = e.m_data[i];
		for (uint32_t j = 0; j < digit_bits; j++)
		{
			if (digit & 1)
			{
				Mulm(result, result, na2, m); //result = result * na2 (mod m)
			}
			digit >>= 1;
			if (i == cnt && !digit)
				break;
			Mulm(na2, na2, na2, m);
		}
	}
	return true;
}

void BigNumber::Set(const uint32_t i)
{
	SetZero();
	if (!m_data.empty())
	{
		m_data[0] = i;
	}
}

bool BigNumber::IsEqual(const BigNumber& other) const
{
	if (Length() != other.Length())
	{
		return false;
	}

	for (uint32_t i = 0; i < Length(); i++)
	{
		if (m_data[i] != other.m_data[i])
		{
			return false;
		}
	}
	return true;
}

int BigNumber::Compare(const BigNumber& right) const
{
	auto len = (int)std::max(Length(), right.Length());

	for (int i = len - 1; i >= 0; i--)
	{
		if ((int)Length() <= i)
		{
			if (right.m_data[i])
				return -1;
		}
		else if ((int)right.Length() <= i)
		{
			if (m_data[i])
				return 1;
		}
		else
		{
			if (m_data[i] < right.m_data[i])
			{
				return (uint32_t)-1;
			}
			if (m_data[i] > right.m_data[i])
			{
				return 1;
			}
		}
	}
	return 0;
}

namespace
{
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
}

bool BigNumber::Mulm(BigNumber& result, const BigNumber& a1, const BigNumber& a2, const BigNumber& m)
{
	if (a1.Length() != a2.Length() || a2.Length() != m.Length())
	{
		return false;
	}

	result.SetLength(m.Length());

	BigNumber tmp;
	tmp.AllocateData(2 * a1.Length());
	BigNumber q;
	q.AllocateData(a1.Length());

	dbi_mul(tmp, a1, a2);
	return dbi_div(q, result, tmp, m);
}

void BigNumber::dbi_mul(BigNumber& dblResult, const BigNumber& a1, const BigNumber& a2)
{
	dblResult.SetLength(a1.Length() + a2.Length());
	dblResult.SetZero();

	if (a1.m_data == a2.m_data)
	{
		BigNumber::Square_h(a1.GetData(), a1.Length(), dblResult.GetData());
	}
	else
	{
		BigNumber::Mul_h(a1.GetData(), a1.Length(), a2.GetData(), a2.Length(), dblResult.GetData());
	}
}

void BigNumber::Square_h(const digit_t* a, size_t len, digit_t* dst)
{
	while (len > 0 && !a[len - 1])
		len--;

	double_digit_t c = 0;
	//1. Addition of the cross products a[i]*a[j] with i!=j
	for (size_t i = 0; i < len; i++)
	{
		c = 0;
		//There is no overflow (2^n - 1) * (2^n - 1) + 2^n - 1 + 2^n - 1 = 2^2n - 1.
		for (size_t j = i + 1; j < len; j++)
		{
			c += mul_words(a[i], a[j]);
			c += dst[i + j];
			dst[i + j] = lo_word(c);
			shiftr_half_word(c);
		}
		dst[i + len] = lo_word(c);
	}

	//2. Multiply by 2(shifting all results 1 bit left)
	c = 0;
	for (size_t i = 1; i < 2 * len; i++)
	{
		c |= (((double_digit_t)dst[i]) << 1);
		dst[i] = lo_word(c);
		shiftr_half_word(c);
	}

	//3. Adding the squares on the even columns and propagating the sum
	c = 0;
	for (size_t i = 0; i < len; i++)
	{
		// Even column
		c += mul_words(a[i], a[i]);
		c += dst[2 * i];
		dst[2 * i] = lo_word(c);
		shiftr_half_word(c);

		// Odd column
		c += dst[2 * i + 1];
		dst[2 * i + 1] = lo_word(c);
		shiftr_half_word(c);
	}
}

void BigNumber::Mul_h(const digit_t* a, size_t len1, const digit_t* b, size_t len2, digit_t* dst)
{
	while (len1 > 0 && !a[len1 - 1])
		len1--;

	while (len2 > 0 && !b[len2 - 1])
		len2--;

	for (size_t i = 0; i < len1; i++)
	{
		double_digit_t c = 0;
		//There is no overflow (2^n - 1) * (2^n - 1) + 2^n - 1 + 2^n - 1 = 2^2n - 1.
		for (size_t j = 0; j < len2; j++)
		{
			c += mul_words(a[i], b[j]);
			c += dst[i + j];
			dst[i + j] = lo_word(c);
			shiftr_half_word(c);
		}
		dst[i + len2] = lo_word(c);
	}
}

//simple left shift, dst_len = src_len + 1
template<typename T>
void shift_left(T* dst, const T* src, const size_t src_len, const uint32_t shift)
{
	constexpr uint32_t shiftMax = 8 * sizeof(T);

	if (shift == 0)
	{
		//no shift, just copy
		memcpy(dst, src, src_len * sizeof(T));
		dst[src_len] = 0;//dst_len = src_len + 1
	}
	else
	{
		dst[src_len] = src[src_len - 1] >> (shiftMax - shift);
		for (size_t i = src_len - 1; i > 0; i--)
			dst[i] = (src[i] << shift) | (src[i - 1] >> (shiftMax - shift));
		dst[0] = src[0] << shift;
	}
}

//simple shift right
template<typename T>
void shift_right(T* dst, const T* src, const size_t len, const uint32_t shift)
{
	constexpr uint32_t shiftMax = 8 * sizeof(T);

	if (shift == 0)
	{
		//no shift, just copy
		memcpy(dst, src, len * sizeof(T));
	}
	else
	{
		for (size_t i = 0; i < len - 1; i++)
			dst[i] = (src[i] >> shift) | (src[i + 1] << (shiftMax - shift));
		dst[len - 1] = src[len - 1] >> shift;
	}
}

bool BigNumber::dbi_div(BigNumber& result, BigNumber& remainder, const BigNumber& dblA, const BigNumber& b)
{
	if (dblA.Length() != 2 * b.Length())
	{
		return false;
	}

	result.SetLength(b.Length());
	remainder.SetLength(b.Length());

	result.SetZero();
	remainder.SetZero();

	const auto* ptrA = dblA.GetData();
	const auto* ptrB = b.GetData();
	auto* ptrQ = result.GetData();
	auto* ptrR = remainder.GetData();

	auto lenA = static_cast<int32_t>(dblA.Length());
	auto lenB = static_cast<int32_t>(b.Length());
	const auto lenQ = static_cast<int32_t>(result.Length());

	while (lenA > 0 && !ptrA[lenA - 1])
		lenA--;

	while (lenB > 0 && !ptrB[lenB - 1])
		lenB--;

	//division by zero
	if (lenB == 0)
		return false;

	//dividend is smaller than the divisor, just copy remainder
	if (lenA < lenB || dblA.Compare(b) == -1)
	{
		memcpy(ptrR, ptrA, lenA * sizeof(digit_t));
		return true;
	}

	//special case - divide by single digit. Algorithm D need lenB > 1 
	if (lenB == 1)
	{
		s_double_digit_t k = 0;
		double_digit_t Q, R = 0;
		for (int32_t j = lenA - 1; j >= 0; j--)
		{
			k = make_dword(ptrA[j], lo_word(k));
			Q = div_rem(k, ptrB[0], R);
			if (j < lenQ)
			{
				ptrQ[j] = lo_word(Q);
			}
			k -= lo_word(Q) * ptrB[0];
		}
		ptrR[0] = lo_word(k);
		return true;
	}

	//The art  of  computer  programming  I Donald  Ervin  Knuth.  -- 3rd  ed.
	//Chapter 4.3.1: Algorithm D (Division  of nonnegative  integers).

	//D1. [Normalize.]
	const auto shift = digit_bits - 1 - BigNumber::MsbPos(ptrB[lenB - 1]);
	BigNumber normA, normB;
	normA.SetLength(lenA + 1);
	normB.SetLength(lenB + 1);

	auto pa = normA.GetData();
	auto pb = normB.GetData();

	shift_left(pa, ptrA, lenA, shift);
	shift_left(pb, ptrB, lenB, shift);

	const auto divisor = pb[lenB - 1];
	const auto divisorLoW = pb[lenB - 2];

	//D2. [Initialize j.]
	for (int32_t j = lenA - lenB; j >= 0; j--)
	{
		//D3. [Calculate q.]
		const auto dividend = make_dword(pa[j + lenB - 1], pa[j + lenB]);
		const auto dividendLoW = pa[j + lenB - 2];
		double_digit_t R;
		double_digit_t Q = div_rem(dividend, divisor, R);

		while (Q >= digit_base || mul_words(lo_word(Q), divisorLoW) > make_dword(dividendLoW, lo_word(R)))
		{
			Q--;
			R += divisor;
			if (R >= digit_base) //and repeat this test if R < base
				break;
		}

		const auto q = lo_word(Q);

		//D4. [Multiply and subtract.]
		s_double_digit_t carry = 0;
		s_double_digit_t subProd = 0;
		for (int32_t i = 0; i < lenB; i++)
		{
			const auto mulProd = mul_words(q, pb[i]);
			subProd = pa[i + j];
			subProd -= lo_word(mulProd);
			subProd -= carry;
			pa[i + j] = lo_word(subProd);
			carry = hi_word(mulProd) - hi_word(subProd);
		}
		subProd = pa[j + lenB] - carry;
		pa[j + lenB] = lo_word(subProd);

		//D5. [Test remainder.]
		if (subProd < 0)
		{
			Q--;
			//D6. [Add back.]
			[[maybe_unused]] const auto c = Add_h(&pa[j], pb, lenB);

			pa[j + lenB]++;
		}

		// Store quotient digit.
		// don't overwrite data over BigInteger length
		// theoretical - result can have lenA - lenB + 1 digits
		// but in RSA we use modular arithmetic in finite field, dividend < mod*mod
		// therefore first digit is always zero.
		if (j < lenQ)
			ptrQ[j] = lo_word(Q);
	}//D7. [Loop on j.]

	//D8. [Unnormalize.]
	shift_right(ptrR, pa, lenB, shift);

	return true;
}

BigNumber::digit_t BigNumber::Add_h(digit_t* a, const digit_t* b, size_t len)
{
	double_digit_t buff = 0;
	for (size_t i = 0; i < len; i++)
	{
		buff += a[i];
		buff += b[i];
		a[i] = lo_word(buff);
		shiftr_half_word(buff);
	}
	return lo_word(buff);
}

BigNumber::digit_t BigNumber::Sub_h(digit_t* a, const digit_t* b, size_t len)
{
	s_double_digit_t buff = 0;
	for (size_t i = 0; i < len; i++)
	{
		buff += a[i];
		buff -= b[i];
		a[i] = lo_word(buff);
		shiftr_half_word(buff);
	}
	return lo_word(buff) == 0 ? 0 : 1;
}

void BigNumber::bi2dbi(BigNumber& dest, const BigNumber& src)
{
	dest.SetLength(2 * src.Length());
	memcpy(dest.GetData(), src.GetData(), src.GetByteSize());
	memset(dest.GetData() + src.Length(), 0, src.GetByteSize());
}

void BigNumber::dbi2bi(BigNumber& dest, const BigNumber& src)
{
	if (0 == src.Length())
	{
		return;
	}

	dest.SetLength(src.Length() / 2);
	memcpy(dest.GetData(), src.GetData(), dest.GetByteSize());
}

BigNumber::digit_t BigNumber::Add(BigNumber& a, const BigNumber& b)
{
	if (a.Length() != b.Length())
	{
		return -1;
	}

	return Add_h(a.GetData(), b.GetData(), a.Length());
}

BigNumber::digit_t BigNumber::Sub(BigNumber& a, const BigNumber& b)
{
	if (a.Length() != b.Length())
	{
		return -1;
	}

	return Sub_h(a.GetData(), b.GetData(), a.Length());
}

bool BigNumber::Div(BigNumber& result, BigNumber& remainder, const BigNumber& a, const BigNumber& b)
{
	if (a.Length() != b.Length())
	{
		return false;
	}

	result.SetLength(b.Length());
	remainder.SetLength(b.Length());

	BigNumber tmp;
	tmp.AllocateData(2 * b.Length());
	bi2dbi(tmp, a);

	return dbi_div(result, remainder, tmp, b);
}

bool BigNumber::Mul(BigNumber& result, const BigNumber& a1, const BigNumber& a2)
{
	if (a1.Length() != a2.Length())
	{
		return false;
	}

	BigNumber tmp;
	tmp.AllocateData(2 * a1.Length());
	dbi_mul(tmp, a1, a2);
	dbi2bi(result, tmp);

	return true;
}

bool BigNumber::operator==(const BigNumber& other) const
{
	return Compare(other) == 0;
}

uint32_t BigNumber::MsbPos(uint32_t v)
{
#if defined(_MSC_VER)
	if (!v)
		return 0;
	unsigned long index = 0;
	_BitScanReverse(&index, v);
	return index;
#else
	const uint32_t b[] = { 0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000 };
	const uint32_t S[] = { 1, 2, 4, 8, 16 };

	uint32_t r = 0;
	for (int32_t i = 4; i >= 0; i--)
	{
		if (v & b[i])
		{
			v >>= S[i];
			r |= S[i];
		}
	}
	return r;
#endif
}

uint32_t BigNumber::MsbPos(uint64_t v)
{
#if defined(_MSC_VER) && defined(_WIN64)
	if (!v)
		return 0;
	unsigned long index = 0;
	_BitScanReverse64(&index, v);
	return index;
#else
	const uint64_t b[] = { 0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000, 0xFFFFFFFF00000000ull };
	const uint64_t S[] = { 1, 2, 4, 8, 16, 32 };

	uint32_t r = 0;
	for (int32_t i = 5; i >= 0; i--)
	{
		if (v & b[i])
		{
			v >>= S[i];
			r |= S[i];
		}
	}
	return r;
#endif
}
