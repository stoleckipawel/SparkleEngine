#pragma once

#include "GameFramework/Public/Rendering/RenderFrameSubmission.h"
#include "World/Extraction/Dynamic/RenderSceneDynamicDataExtractor.h"
#include "World/Extraction/Identity/RenderObjectIdentityMap.h"
#include "World/Extraction/Resources/RenderResourcePublisher.h"
#include "World/Extraction/Structural/RenderObjectDeltaExtractor.h"

#include <cstdint>

class GameWorldResourceStores;
class WorldReadView;

namespace ECS
{
	class GameWorldState;

	class RenderFrameSubmissionExtractor final
	{
	public:
		RenderFrameSubmission Extract(
		    GameWorldState& state,
		    GameWorldResourceStores& resources,
		    const WorldReadView& readView,
		    std::uint64_t sceneGeneration,
		    std::uint64_t frameId);

	private:
		void BeginFrame(RenderFrameSubmission& submission, std::uint64_t sceneGeneration, std::uint64_t frameId);
		static RenderViewCameraData BuildViewCamera(const WorldReadView& readView) noexcept;

		RenderObjectIdentityMap m_identities;
		RenderObjectDeltaExtractor m_objectExtractor;
		RenderResourcePublisher m_resourcePublisher;
		RenderSceneDynamicDataExtractor m_dynamicExtractor;
		std::uint64_t m_sequence = 0;
		std::uint64_t m_sceneGeneration = 0;
	};
}
