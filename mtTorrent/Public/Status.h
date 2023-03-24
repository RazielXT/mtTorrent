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
		I_Merged,
		I_AlreadyExists,
		I_Mismatch,

		E_Unknown = 1000,
		E_InvalidInput,
		E_Unsupported,
		E_NotEnoughSpace,
		E_AllocationProblem,
		E_InvalidPath,
		E_NoData,
		E_FileReadError,
		E_FileWriteError,
		E_FileLockedError,
		E_NotEmpty,
		E_Unwanted,

		E_NetworkError = 2000,
		E_ConnectionClosed
	};
}