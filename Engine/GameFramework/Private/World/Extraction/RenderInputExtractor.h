#pragma once

#include "GameFramework/Public/Rendering/RenderInputFrame.h"
#include "World/Extraction/Dynamic/RenderFrameDynamicDataExtractor.h"
#include "World/Extraction/Identity/RenderObjectIdentityMap.h"
#include "World/Extraction/Resources/RenderResourcePublisher.h"
#include "World/Extraction/Structural/RenderObjectDeltaExtractor.h"

#include <cstdint>

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
		void BeginFrame(RenderInputFrame& frame, std::uint64_t sceneGeneration, RenderFrameMetadata metadata);

		RenderObjectIdentityMap m_identities;
		RenderObjectDeltaExtractor m_objectExtractor;
		RenderResourcePublisher m_resourcePublisher;
		RenderFrameDynamicDataExtractor m_dynamicExtractor;
		std::uint64_t m_sequence = 0;
		std::uint64_t m_sceneGeneration = 0;
	};
}
