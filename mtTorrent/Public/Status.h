#pragma once

namespace mtt
{
	enum class Status
	{
		Success,

		I_Starting,
		I_Connecting,
		I_Requesting,
		I_Stopped,

		E_Unknown = 1000,
		E_InvalidInput,
		E_Unsupported,
		E_NotEnoughSpace,
		E_AllocationProblem,
		E_NoData,

		E_NetworkError = 2000,
		E_ConnectionClosed
	};
}