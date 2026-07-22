#pragma once

#include "GameFramework/Public/Rendering/RenderFrameDynamicData.h"
#include "World/Extraction/WorldExtractionStorage.h"

#include <span>

class GameWorldResourceStores;
class WorldReadView;

namespace ECS
{
	class GameWorldState;
	class RenderObjectDeltaExtractor;
	class RenderObjectIdentityMap;
	class RenderFrameDynamicDataExtractor final
	{
	  public:
		void Extract(
		    GameWorldState& state,
		    const GameWorldResourceStores& resources,
		    const WorldReadView& readView,
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    const RenderObjectDeltaExtractor& objects,
		    RenderObjectIdentityMap& identities,
		    RenderFrameDynamicData& dynamic) const;

	  private:
		static void ExtractObjects(
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    const RenderObjectDeltaExtractor& objects,
		    RenderFrameDynamicData& dynamic);
		static void ExtractSkinning(
		    GameWorldState& state,
		    const GameWorldResourceStores& resources,
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    const RenderObjectDeltaExtractor& objects,
		    RenderFrameDynamicData& dynamic);
		static void ExtractMorphWeights(
		    GameWorldState& state,
		    const RenderObjectDeltaExtractor& objects,
		    RenderFrameDynamicData& dynamic);
		static void ExtractLights(
		    const WorldReadView& readView,
		    RenderObjectIdentityMap& identities,
		    RenderFrameDynamicData& dynamic);
		static RenderCameraData BuildCamera(const WorldReadView& readView) noexcept;
	};
}
