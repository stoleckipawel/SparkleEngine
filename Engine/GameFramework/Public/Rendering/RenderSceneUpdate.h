#pragma once

#include "GameFramework/Public/Rendering/RenderSceneDelta.h"
#include "GameFramework/Public/Rendering/RenderSceneDynamicData.h"

struct RenderSceneUpdate final
{
	RenderSceneDelta Structural;
	RenderSceneDynamicData Dynamic;
};
