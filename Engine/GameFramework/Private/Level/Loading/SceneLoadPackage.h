#pragma once

#include "Assets/SceneAssetPayload.h"
#include "Level/LevelDesc.h"
#include "World/ECS/ComponentSchema.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Assets
{
	struct EntityBlueprint final
	{
		std::string AuthoredIdentity;
		std::vector<ECS::ComponentSchema> Components;
	};

	struct SceneLoadPackage final
	{
		std::uint64_t RequestId = 0;
		std::uint64_t WorldGeneration = 0;
		std::uint64_t DocumentGeneration = 0;
		std::uint64_t CatalogGeneration = 0;
		LevelDesc Level;
		std::vector<SceneAssetPayload> AssetPayloads;
		std::vector<EntityBlueprint> Entities;
	};
}
