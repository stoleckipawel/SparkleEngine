#pragma once

#include "GameFramework/Public/Scene/Materials/MaterialDesc.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <vector>
#include <cstdint>

struct SPARKLE_ENGINE_API MaterialSnapshot
{
	std::vector<MaterialDesc> materialDescs;
	std::uint32_t generation = 0;

	bool HasMaterials() const noexcept { return !materialDescs.empty(); }

	void Reset() noexcept { materialDescs.clear(); generation = 0; }
};
