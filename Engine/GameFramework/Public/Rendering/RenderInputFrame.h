#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Rendering/RenderFrameDynamicData.h"
#include "GameFramework/Public/Rendering/RenderWorldDelta.h"

struct SPARKLE_ENGINE_API RenderInputFrame final
{
	RenderWorldDelta WorldDelta;
	RenderFrameDynamicData Dynamic;
};
