// ============================================================================
// CesiumManLevel.h — Cesium Man glTF test scene
// ============================================================================

#pragma once

#include "Level/Level.h"

class CesiumManLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "CesiumMan"; }

	std::string_view GetDescription() const override { return "Cesium Man — imported glTF character scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		MeshRequest req;
		req.source = AssetSource::Imported;
		req.assetPath = "CesiumMan/CesiumMan.gltf";
		req.assetType = AssetType::Mesh;
		desc.meshRequests.push_back(req);
		return desc;
	}
};