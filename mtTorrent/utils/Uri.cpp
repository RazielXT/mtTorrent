#include "Uri.h"
#include <algorithm>

Uri Uri::Parse(const std::string& uri)
{
	Uri result;
	using iterator_t = std::string::const_iterator;

	if (uri.length() == 0)
		return result;

	iterator_t uriEnd = uri.end();

	// protocol
	iterator_t protocolStart = uri.begin();
	iterator_t protocolEnd = std::find(protocolStart, uriEnd, L':');

	if (protocolEnd != uriEnd)
	{
		std::string prot = &*(protocolEnd);
		if ((prot.length() > 3) && (prot.compare(0, 3, "://") == 0))
		{
			result.protocol = std::string(protocolStart, protocolEnd);
			protocolEnd += 3;
		}
		else
			protocolEnd = uri.begin();
	}
	else
		protocolEnd = uri.begin();

	// host
	iterator_t hostStart = protocolEnd;
	iterator_t pathStart = std::find(hostStart, uriEnd, '/');  // get pathStart
	iterator_t hostEnd = std::find(protocolEnd, pathStart, ':');  // check for port

	result.host = std::string(hostStart, hostEnd);

	// port
	if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == L':'))
	{
		hostEnd++;
		result.port = std::string(hostEnd, pathStart);
	}

	// path
	if (pathStart != uriEnd)
		result.path = std::string(pathStart, uriEnd);

	return result;
}
