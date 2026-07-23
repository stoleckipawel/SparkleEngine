#pragma once

#include "Rendering/RenderInputFrame.h"
#include "Time/Timer.h"

struct RenderFramePacket final
{
	RenderInputFrame Input;
	TimeInfo Timing;
};
