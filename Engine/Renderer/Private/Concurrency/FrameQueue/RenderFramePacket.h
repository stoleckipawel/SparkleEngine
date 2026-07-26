#pragma once

#include "Rendering/RenderInputFrame.h"
#include "Renderer/Public/UI/UiRenderPacket.h"
#include "Time/Timer.h"

struct RenderFramePacket final
{
	RenderInputFrame Input;
	TimeInfo Timing;
	UiRenderPacket Ui;
};
