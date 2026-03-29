#pragma once

#include "Renderer/Public/RendererAPI.h"

#include <DirectXMath.h>
#include <cstdint>

class GPUMesh;

struct SPARKLE_RENDERER_API MeshDraw
{
	DirectX::XMFLOAT4X4 worldMatrix = {};
	DirectX::XMFLOAT3X4 worldInvTranspose = {};
	std::uint32_t materialSlot = 0;
	const GPUMesh* gpuMesh = nullptr;
};
