#pragma once

#include "GameFramework/Public/Rendering/RenderWorldDelta.h"
#include "GameFramework/Public/World/EntityId.h"
#include "World/Extraction/WorldExtractionStorage.h"

#include <map>
#include <span>

namespace ECS
{
	class RenderObjectIdentityMap;
	class RenderObjectDeltaExtractor final
	{
	  public:
		void BeginScene() noexcept { m_published.clear(); }
		void Extract(
		    std::span<const WorldExtractionStorage::MeshSlot> meshes,
		    RenderObjectIdentityMap& identities,
		    RenderWorldDelta& delta);
		RenderObjectId FindObject(EntityId entity) const noexcept;

	  private:
		struct PublishedObject final
		{
			RenderObjectId Object;
			RenderObjectStaticData Static;
		};

		static bool HasSameStaticData(
		    const RenderObjectStaticData& left,
		    const RenderObjectStaticData& right) noexcept;

		std::map<EntityId, PublishedObject> m_published;
	};
}
