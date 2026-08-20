#pragma once

#include "Scene/Preparation/RenderDeformationPreparation.h"
#include "Scene/Preparation/RenderLightPreparation.h"
#include "Scene/Preparation/RenderPrimitivePreparation.h"
#include "Scene/Preparation/PreparedRenderScene.h"

#include <span>
#include <vector>

struct RenderScenePreparationRun final
{
	std::span<const RenderLightData> Lights;
	PreparedRenderScene PreparedScene;
	std::vector<ResolvedRenderPrimitive> ResolvedPrimitives;
	std::vector<PreparedRenderPrimitive> PreparedPrimitives;
	std::vector<PreparedRenderLight> PreparedLights;
	RenderDeformationWork Deformation;
};
