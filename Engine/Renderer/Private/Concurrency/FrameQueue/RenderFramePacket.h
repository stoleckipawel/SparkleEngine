#pragma once

#include "Rendering/RenderInputFrame.h"
#include "Renderer/Public/Editor/EditorRenderPacket.h"
#include "Time/Timer.h"

struct RenderFramePacket final
{
	RenderInputFrame Input;
	TimeInfo Timing;
	EditorRenderPacket EditorUi;
};
