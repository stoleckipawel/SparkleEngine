#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"

#include <DirectXMath.h>
#include <cstdint>
#include <vector>

class Mesh;

struct RenderSceneMeshRecord
{
	const Mesh* mesh = nullptr;
	DirectX::XMFLOAT4X4 worldMatrix = {};
	DirectX::XMFLOAT3X4 worldInvTranspose = {};
	std::uint32_t materialId = 0;
};

struct RenderSceneSnapshot
{
	std::vector<RenderSceneMeshRecord> meshes;
	std::vector<MaterialDesc> materials;

	bool HasMeshes() const noexcept { return !meshes.empty(); }
};