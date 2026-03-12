#pragma once

#include "Renderer/Public/RendererAPI.h"

#include <DirectXMath.h>
#include <cstdint>

struct SPARKLE_RENDERER_API MeshDraw
{
	DirectX::XMFLOAT4X4 worldMatrix = {};
	DirectX::XMFLOAT3X4 worldInvTranspose = {};
	std::uint32_t materialId = 0;
	const void* meshPtr = nullptr;
};
