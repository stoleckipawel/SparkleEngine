// ============================================================================
// ABeautifulGameLevel.h — A Beautiful Game glTF test scene
// ============================================================================

#pragma once

#include "Level/Level.h"

class ABeautifulGameLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "ABeautifulGame"; }

	std::string_view GetDescription() const override { return "A Beautiful Game — imported glTF showcase scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		MeshRequest req;
		req.source = AssetSource::Imported;
		req.assetPath = "ABeautifulGame/ABeautifulGame.gltf";
		req.assetType = AssetType::Mesh;
		desc.meshRequests.push_back(req);
		return desc;
	}
};