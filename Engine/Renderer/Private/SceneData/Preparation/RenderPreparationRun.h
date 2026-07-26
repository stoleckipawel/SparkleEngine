#pragma once

#include "Core/Public/Math/Frustum.h"
#include "SceneData/Preparation/RenderDeformationPreparation.h"
#include "SceneData/Preparation/RenderLightPreparation.h"
#include "SceneData/Preparation/RenderObjectPreparation.h"
#include "SceneData/RenderSceneData.h"

#include <span>
#include <vector>

struct RenderPreparationRun final
{
	Frustum ViewFrustum;
	DirectX::XMFLOAT3 CameraPosition = {};
	std::span<const RenderLightData> Lights;
	RenderSceneData SceneData;
	std::vector<ResolvedRenderObject> ResolvedObjects;
	std::vector<PreparedRenderObject> PreparedObjects;
	std::vector<PreparedRenderLight> PreparedLights;
	std::vector<RenderMeshInstanceGroup> InstanceGroups;
	RenderDeformationWork Deformation;
	bool EnableAutoBatching = false;
};
