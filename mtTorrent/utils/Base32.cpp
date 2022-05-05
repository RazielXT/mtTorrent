#include "Base32.h"
#include <array>
#include <cctype>
#include <iterator>

std::string base32decode(const std::string& s)
{
	std::array<std::uint8_t, 8> inbuf;
	std::array<std::uint8_t, 5> outbuf;

	std::string ret;
	for (auto i = s.begin(); i != s.end();)
	{
		int available_input = std::min(int(inbuf.size()), int(s.end() - i));

		int pad_start = 0;
		if (available_input < 8) pad_start = available_input;

		// clear input buffer
		inbuf.fill(0);
		for (int j = 0; j < available_input; ++j)
		{
			char const in = char(std::toupper(*i++));
			if (in >= 'A' && in <= 'Z')
				inbuf[j] = (in - 'A') & 0xff;
			else if (in >= '2' && in <= '7')
				inbuf[j] = (in - '2' + ('Z' - 'A') + 1) & 0xff;
			else if (in == '=')
			{
				inbuf[j] = 0;
				if (pad_start == 0) pad_start = j;
			}
			else if (in == '1')
				inbuf[j] = 'I' - 'A';
			else
				return {};
		}

		// decode inbuf to outbuf
		outbuf[0] = (inbuf[0] << 3) & 0xff;
		outbuf[0] |= inbuf[1] >> 2;
		outbuf[1] = ((inbuf[1] & 0x3) << 6) & 0xff;
		outbuf[1] |= inbuf[2] << 1;
		outbuf[1] |= (inbuf[3] & 0x10) >> 4;
		outbuf[2] = ((inbuf[3] & 0x0f) << 4) & 0xff;
		outbuf[2] |= (inbuf[4] & 0x1e) >> 1;
		outbuf[3] = ((inbuf[4] & 0x01) << 7) & 0xff;
		outbuf[3] |= (inbuf[5] & 0x1f) << 2;
		outbuf[3] |= (inbuf[6] & 0x18) >> 3;
		outbuf[4] = ((inbuf[6] & 0x07) << 5) & 0xff;
		outbuf[4] |= inbuf[7];

		const int input_output_mapping[] = { 5, 1, 1, 2, 2, 3, 4, 4, 5 };
		int num_out = input_output_mapping[pad_start];

		// write output
		std::copy(outbuf.begin(), outbuf.begin() + num_out, std::back_inserter(ret));
	}
	return ret;
}