#pragma once

#include "GameFramework/Public/Rendering/RenderInputFrame.h"
#include "GameFramework/Public/World/EntityId.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>

class GameWorldResourceStores;
class WorldReadView;

namespace ECS
{
	class GameWorldState;

	class RenderInputExtractor final
	{
	  public:
		RenderInputFrame Extract(
		    GameWorldState& state,
		    GameWorldResourceStores& resources,
		    const WorldReadView& readView,
		    std::uint64_t sceneGeneration,
		    RenderFrameMetadata metadata);

	  private:
		struct PublishedObject final
		{
			RenderObjectId Object;
			MaterialHandle Material = MaterialHandle::Invalid();
			SceneMeshInstanceGroupIndex InstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
		};

		void BeginSceneFrame(RenderInputFrame& frame, std::uint64_t sceneGeneration, RenderFrameMetadata metadata);
		void ExtractObjects(GameWorldState& state, RenderInputFrame& frame);
		void ExtractResources(GameWorldState& state, GameWorldResourceStores& resources, RenderWorldDelta& delta);
		static void ExtractInstanceGroups(GameWorldState& state, RenderWorldDelta& delta);
		void PublishChangedResourceTables(
		    GameWorldResourceStores& resources,
		    const std::optional<std::filesystem::path>& skyTexturePath,
		    RenderWorldDelta& delta);
		void ExtractDynamicState(GameWorldState& state, const WorldReadView& readView, RenderFrameDynamicData& dynamic) const;
		static RenderCameraData BuildCamera(const WorldReadView& readView) noexcept;

		std::map<EntityId, PublishedObject> m_objects;
		std::uint32_t m_nextObjectValue = 0;
		std::uint64_t m_sequence = 0;
		std::uint64_t m_sceneGeneration = 0;
		std::uint64_t m_publishedMaterialRevision = 0;
		std::uint64_t m_publishedTextureRevision = 0;
		std::optional<std::filesystem::path> m_publishedSkyTexturePath;
	};
}
