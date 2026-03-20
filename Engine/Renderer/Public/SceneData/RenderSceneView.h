#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"

#include <cstdint>
#include <vector>

class RenderCamera;

struct SPARKLE_RENDERER_API RenderSceneView
{
	const RenderCamera* camera = nullptr;

	std::uint32_t width = 0;
	std::uint32_t height = 0;

	std::vector<DirectionalLight> directionalLights;

	std::vector<MeshDraw> meshDraws;
	std::vector<MaterialData> materials;

	std::uint32_t GetDirectionalLightCount() const noexcept { return static_cast<std::uint32_t>(directionalLights.size()); }
};
