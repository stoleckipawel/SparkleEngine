// ============================================================================
// DamagedHelmetLevel.h — Damaged Helmet glTF test scene
// ============================================================================

#pragma once

#include "Level/Level.h"

class DamagedHelmetLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "DamagedHelmet"; }

	std::string_view GetDescription() const override { return "Damaged Helmet — imported glTF material test scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		MeshRequest req;
		req.source = AssetSource::Imported;
		req.assetPath = "DamagedHelmet/DamagedHelmet.gltf";
		req.assetType = AssetType::Mesh;
		desc.meshRequests.push_back(req);
		return desc;
	}
};