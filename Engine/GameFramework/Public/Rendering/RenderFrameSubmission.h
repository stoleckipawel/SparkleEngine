#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Rendering/RenderSceneUpdate.h"
#include "GameFramework/Public/Rendering/RenderViewInput.h"

#include <cstdint>

struct SPARKLE_ENGINE_API RenderFrameSubmission final
{
	std::uint64_t FrameId = 0;
	RenderSceneUpdate Scene;
	RenderViewInput View;
};
