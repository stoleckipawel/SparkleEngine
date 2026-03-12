#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/MeshFactory.h"  // MeshFactory::Shape
#include "GameFramework/Public/Assets/AssetTypes.h"  // AssetType

#include <DirectXMath.h>
#include <cstdint>
#include <filesystem>
#include <vector>

struct SPARKLE_ENGINE_API PrimitiveRequest
{
	MeshFactory::Shape shape = MeshFactory::Shape::Box;
	std::uint32_t count = 500;
	DirectX::XMFLOAT3 center = {0.0f, 0.0f, 50.0f};
	DirectX::XMFLOAT3 extents = {100.0f, 100.0f, 100.0f};
	std::uint32_t seed = 1337;
};

// Mesh request - unified path for imported and procedural meshes.
enum class AssetSource
{
	Imported,
	Procedural,
};

struct SPARKLE_ENGINE_API MeshRequest
{
	AssetSource source = AssetSource::Imported;
	AssetType assetType = AssetType::Mesh;

	// Imported mesh (used when source == Imported).
	std::filesystem::path assetPath;  // Relative (e.g., "Sponza/Sponza.gltf")

	// Procedural mesh (used when source == Procedural).
	PrimitiveRequest procedural;
};

// Complete declarative level description.
struct SPARKLE_ENGINE_API LevelDesc
{
	std::vector<MeshRequest> meshRequests;
	// Future: std::vector<LightDesc> lights;
};
