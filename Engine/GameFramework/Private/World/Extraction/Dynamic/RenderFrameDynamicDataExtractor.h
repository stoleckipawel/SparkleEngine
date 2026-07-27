#pragma once

#include "GameFramework/Public/Rendering/RenderFrameDynamicData.h"
#include "World/Extraction/WorldExtractionStorage.h"

#include <limits>
#include <span>
#include <vector>

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
		void BeginScene() noexcept;
		void Extract(
		    GameWorldState& state,
		    const GameWorldResourceStores& resources,
		    const WorldReadView& readView,
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    const RenderObjectDeltaExtractor& objects,
		    RenderObjectIdentityMap& identities,
		    RenderFrameDynamicData& dynamic);

	  private:
		struct MorphMetadata final
		{
			EntityId Entity;
			RenderAnimationAssetHandle Animation;
			std::uint32_t TargetNodeIndex =
			    (std::numeric_limits<std::uint32_t>::max)();
		};

		void ExtractObjects(
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    const RenderObjectDeltaExtractor& objects,
		    RenderFrameDynamicData& dynamic);
		static bool HasSameObjectData(
		    const RenderObjectDynamicData& left,
		    const RenderObjectDynamicData& right) noexcept;
		static void ExtractSkinning(
		    GameWorldState& state,
		    const GameWorldResourceStores& resources,
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    const RenderObjectDeltaExtractor& objects,
		    RenderFrameDynamicData& dynamic);
		void ExtractMorphWeights(
		    GameWorldState& state,
		    const RenderObjectDeltaExtractor& objects,
		    RenderFrameDynamicData& dynamic);
		static void ExtractLights(
		    const WorldReadView& readView,
		    RenderObjectIdentityMap& identities,
		    RenderFrameDynamicData& dynamic);
		static RenderCameraData BuildCamera(const WorldReadView& readView) noexcept;

		std::vector<RenderObjectDynamicData> m_publishedObjects;
		std::vector<RenderObjectDynamicData> m_currentObjects;
		std::vector<MorphMetadata> m_morphMetadata;
	};
}
