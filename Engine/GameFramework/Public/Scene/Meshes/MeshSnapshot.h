#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <vector>

class MeshComponent;

struct SPARKLE_ENGINE_API MeshSnapshot
{
	std::vector<const MeshComponent*> meshComponents;

	bool HasMeshes() const noexcept { return !meshComponents.empty(); }
	void Reset() noexcept { meshComponents.clear(); }
};