#include "RC4.h"
#include <memory>

void RC4::init(const unsigned char* in, size_t len)
{
	const size_t key_size = sizeof(buf);
	uint8_t key[key_size];

	if (len > key_size) len = key_size;

	xs = 0;
	while (len--) {
		buf[xs++] = *in++;
	}

	uint8_t* s = buf;
	memcpy(key, s, key_size);
	int keylen = xs;

	uint8_t tmp;
	int x, y, j;
	for (x = 0; x < int(key_size); ++x) {
		s[x] = x & 0xff;
	}

	for (j = x = y = 0; x < int(key_size); x++) {
		y = (y + buf[x] + key[j++]) & 255;
		if (j == keylen) {
			j = 0;
		}
		tmp = s[x]; s[x] = s[y]; s[y] = tmp;
	}
	xs = 0;
	ys = 0;
}

void RC4::decode(unsigned char* out, size_t outlen)
{
	uint8_t x, y, tmp;
	uint8_t* s = buf;

	x = xs & 0xff;
	y = ys & 0xff;
	while (outlen--)
	{
		x = (x + 1) & 255;
		y = (y + s[x]) & 255;
		tmp = s[x]; s[x] = s[y]; s[y] = tmp;
		tmp = (s[x] + s[y]) & 255;
		*out++ ^= s[tmp];
	}
	xs = x;
	ys = y;
}

void RC4::encode(unsigned char* out, size_t outlen)
{
	decode(out, outlen);
}

void RC4::skip(size_t sz)
{
	uint8_t x, y, tmp;
	uint8_t* s = buf;

	x = xs & 0xff;
	y = ys & 0xff;
	while (sz--)
	{
		x = (x + 1) & 255;
		y = (y + s[x]) & 255;
		tmp = s[x]; s[x] = s[y]; s[y] = tmp;
		tmp = (s[x] + s[y]) & 255;
	}
	xs = x;
	ys = y;
}
