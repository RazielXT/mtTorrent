#pragma once

#include "ExtensionProtocol.h"
#include "utils/DataBuffer.h"

namespace mtt
{
	namespace ext
	{
		struct UtMetadata : public RemoteExtension
		{
			const static uint32_t PieceSize = 16 * 1024;

			enum class Type
			{
				Request = 0,
				Data,
				Reject
			};

			struct Message
			{
				Type type;
				uint32_t piece;
				BufferView metadata;
				uint32_t totalSize;
			};

			static bool Load(const BufferView& data, Message& msg);

			void sendPieceRequest(uint32_t index);
			void sendPieceResponse(uint32_t index, BufferView data, size_t totalSize);
		};
	}
}
