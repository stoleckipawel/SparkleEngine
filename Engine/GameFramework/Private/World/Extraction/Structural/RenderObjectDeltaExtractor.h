#pragma once

#include "GameFramework/Public/Rendering/RenderWorldDelta.h"
#include "GameFramework/Public/World/EntityId.h"
#include "World/Extraction/WorldExtractionStorage.h"

#include <span>
#include <vector>

namespace ECS
{
	class RenderObjectIdentityMap;
	class RenderObjectDeltaExtractor final
	{
	  public:
		void BeginScene() noexcept;
		void Extract(
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    RenderObjectIdentityMap& identities,
		    RenderWorldDelta& delta);
		RenderObjectId FindObject(EntityId entity) const noexcept;

	  private:
		struct PublishedObject final
		{
			EntityId Entity;
			RenderObjectId Object;
			RenderObjectStaticData Static;
		};

		void RetirePublishedBefore(
		    EntityId entity,
		    std::size_t& publishedIndex,
		    RenderWorldDelta& delta) const;
		PublishedObject ResolveObject(
		    const WorldExtractionStorage::MeshSlot& mesh,
		    RenderObjectIdentityMap& identities,
		    std::size_t& publishedIndex,
		    RenderWorldDelta& delta) const;
		void RetireRemaining(
		    std::size_t publishedIndex,
		    RenderWorldDelta& delta) const;
		static void SortDeltaObjects(RenderWorldDelta& delta);
		static bool HasSameStaticData(
		    const RenderObjectStaticData& left,
		    const RenderObjectStaticData& right) noexcept;

		std::vector<PublishedObject> m_published;
		std::vector<PublishedObject> m_current;
	};
}
