// ============================================================================
// DiffuseTransmissionPlantLevel.h — Diffuse Transmission Plant glTF test scene
// ============================================================================

#pragma once

#include "Level/Level.h"

class DiffuseTransmissionPlantLevel final : public Level
{
  public:
	std::string_view GetName() const override { return "DiffuseTransmissionPlant"; }

	std::string_view GetDescription() const override { return "Diffuse Transmission Plant — imported glTF foliage test scene"; }

	LevelDesc BuildDescription() const override
	{
		LevelDesc desc;
		MeshRequest req;
		req.source = AssetSource::Imported;
		req.assetPath = "DiffuseTransmissionPlant/DiffuseTransmissionPlant.gltf";
		req.assetType = AssetType::Mesh;
		desc.meshRequests.push_back(req);
		return desc;
	}
};