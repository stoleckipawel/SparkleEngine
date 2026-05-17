#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"

#include <DirectXMath.h>

#include <vector>

class Mesh;

struct SPARKLE_ENGINE_API MeshInstanceSnapshot
{
	const Mesh* mesh = nullptr;
	DirectX::XMFLOAT4X4 worldMatrix = {};
	DirectX::XMFLOAT3X4 worldInvTranspose = {};
	MaterialHandle materialHandle = MaterialHandle::Invalid();
};

struct SPARKLE_ENGINE_API MeshSnapshot
{
	std::vector<MeshInstanceSnapshot> meshInstances;

	bool HasMeshes() const noexcept { return !meshInstances.empty(); }
	void Reset() noexcept { meshInstances.clear(); }
};