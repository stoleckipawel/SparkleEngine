#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/MaterialData.h"
#include "Renderer/Public/SceneData/MeshDraw.h"

#include <cstdint>
#include <vector>

class RenderCamera;

struct SPARKLE_RENDERER_API SceneView
{
	const RenderCamera* camera = nullptr;

	std::uint32_t width = 0;
	std::uint32_t height = 0;

	DirectionalLight sunLight = {};

	std::vector<MeshDraw> meshDraws;
	std::vector<MaterialData> materials;
};
