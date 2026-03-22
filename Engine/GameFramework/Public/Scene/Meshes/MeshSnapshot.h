#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <vector>

class Mesh;

struct SPARKLE_ENGINE_API MeshSnapshot
{
	std::vector<const Mesh*> meshPointers;

	bool HasMeshes() const noexcept { return !meshPointers.empty(); }
	void Reset() noexcept { meshPointers.clear(); }
};