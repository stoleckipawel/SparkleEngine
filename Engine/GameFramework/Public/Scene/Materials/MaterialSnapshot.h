#pragma once

#include "GameFramework/Public/Assets/MaterialDesc.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <vector>

struct SPARKLE_ENGINE_API MaterialSnapshot
{
	std::vector<MaterialDesc> materialDescs;

	bool HasMaterials() const noexcept { return !materialDescs.empty(); }

	void Reset() noexcept
	{
		materialDescs.clear();
	}
};