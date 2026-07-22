#pragma once

#include "GameFramework/Public/Rendering/RenderAssetHandles.h"
#include "GameFramework/Public/Rendering/RenderObjectId.h"
#include "GameFramework/Public/Rendering/RenderResourceTables.h"
#include "GameFramework/Public/Scene/Materials/MaterialHandle.h"
#include "GameFramework/Public/Scene/Meshes/MeshInstanceGroup.h"
#include "GameFramework/Public/Scene/Meshes/SceneMeshKind.h"
#include "GameFramework/Public/Scene/Sky/SceneSkyDesc.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

struct RenderObjectStaticData final
{
	ImmutableRenderMeshHandle Mesh;
	MaterialHandle Material = MaterialHandle::Invalid();
	RenderSkeletonAssetHandle Skeleton;
	SceneMeshKind MeshKind = SceneMeshKind::Static;
	SceneMeshAssetIndex MeshAssetIndex = kInvalidSceneMeshAssetIndex;
	SceneMeshInstanceGroupIndex InstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
};

struct RenderObjectCreate final
{
	RenderObjectId Object;
	RenderObjectStaticData Static;
};

struct RenderObjectUpdate final
{
	RenderObjectId Object;
	RenderObjectStaticData Static;
};

struct RenderMeshInstanceGroupData final
{
	Assets::CookedAssetId MeshAssetId = Assets::InvalidCookedAssetId;
	SceneMeshAssetIndex MeshAssetIndex = kInvalidSceneMeshAssetIndex;
	MaterialHandle Material = MaterialHandle::Invalid();
	std::uint32_t FirstInstance = (std::numeric_limits<std::uint32_t>::max)();
	std::uint32_t InstanceCount = 0;
	SceneMeshInstanceGroupKind Kind = SceneMeshInstanceGroupKind::None;
	std::uint32_t Flags = 0;
};

struct RenderInstanceGroupPublication final
{
	bool Published = false;
	std::vector<RenderMeshInstanceGroupData> Values;
};

struct RenderSkyPublication final
{
	bool Published = false;
	std::optional<SceneSkyDesc> Value;
};

struct RenderWorldDelta final
{
	std::uint64_t SceneGeneration = 0;
	std::uint64_t SequenceNumber = 0;
	bool ResetScene = false;
	std::vector<RenderObjectCreate> Creates;
	std::vector<RenderObjectUpdate> Updates;
	std::vector<RenderObjectId> Destroys;
	// Immutable resource tables are structural publications. They are present on a
	// scene reset, not recopied through every dynamic frame packet.
	std::optional<RenderMaterialTable> Materials;
	std::optional<RenderTextureTable> Textures;
	RenderInstanceGroupPublication InstanceGroups;
	RenderSkyPublication Sky;
};
