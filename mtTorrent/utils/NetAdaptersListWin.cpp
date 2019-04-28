#include "NetAdaptersList.h"
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

std::vector<NetAdapters::NetAdapter> NetAdapters::getActiveNetAdapters()
{
	std::vector<NetAdapters::NetAdapter> out;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));

	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
	}

	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
	{
		PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			NetAdapters::NetAdapter netAdapter;
			netAdapter.name = pAdapter->Description;
			netAdapter.clientIp = pAdapter->IpAddressList.IpAddress.String;
			if (pAdapter->GatewayList.Context)
				netAdapter.gateway = pAdapter->GatewayList.IpAddress.String;

			out.push_back(netAdapter);
			pAdapter = pAdapter->Next;
		}
	}

	if (pAdapterInfo)
		free(pAdapterInfo);

	return out;
}
