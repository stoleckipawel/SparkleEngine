#pragma once

#include "Concurrency/Control/RendererExecutionControl.h"
#include "Concurrency/FrameQueue/RenderFrameQueue.h"

#include <cstdint>
#include <variant>

struct RenderFrameReadyCommand final
{
	RenderFrameQueueTicket Ticket;
};

using RenderThreadCommandPayload = std::variant<RenderFrameReadyCommand, RendererExecutionControl>;

struct RenderThreadCommand final
{
	std::uint64_t SequenceNumber = 0;
	RenderThreadCommandPayload Payload;
};
