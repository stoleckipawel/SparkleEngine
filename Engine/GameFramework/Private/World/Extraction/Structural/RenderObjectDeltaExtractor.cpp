#include "PCH.h"
#include "World/Extraction/Structural/RenderObjectDeltaExtractor.h"

#include "World/Extraction/Identity/RenderObjectIdentityMap.h"
#include "World/Extraction/WorldExtractionStorage.h"

namespace ECS
{
	void RenderObjectDeltaExtractor::Extract(
	    std::span<const WorldExtractionStorage::MeshSlot> meshes,
	    RenderObjectIdentityMap& identities,
	    RenderWorldDelta& delta)
	{
		std::map<EntityId, PublishedObject> current;
		for (const WorldExtractionStorage::MeshSlot& mesh : meshes)
		{
			const RenderObjectStaticData staticData{
			    .Mesh = mesh.Mesh,
			    .Material = mesh.Material,
			    .Skeleton = mesh.Skeleton,
			    .MeshKind = mesh.Kind,
			    .MeshAssetIndex = mesh.MeshAssetIndex,
			    .InstanceGroupIndex = mesh.InstanceGroupIndex};

			const auto previous = m_published.find(mesh.Entity);
			PublishedObject published;
			if (previous == m_published.end())
			{
				published = {identities.Resolve(mesh.Entity), staticData};
				delta.Creates.push_back({published.Object, published.Static});
			}
			else
			{
				published = previous->second;
				if (!HasSameStaticData(published.Static, staticData))
				{
					published.Static = staticData;
					delta.Updates.push_back({published.Object, published.Static});
				}
			}
			current.emplace(mesh.Entity, published);
		}

		for (const auto& [entity, published] : m_published)
			if (!current.contains(entity)) delta.Destroys.push_back(published.Object);
		m_published = std::move(current);
	}

	RenderObjectId RenderObjectDeltaExtractor::FindObject(EntityId entity) const noexcept
	{
		const auto object = m_published.find(entity);
		return object == m_published.end() ? RenderObjectId{} : object->second.Object;
	}

	bool RenderObjectDeltaExtractor::HasSameStaticData(
	    const RenderObjectStaticData& left,
	    const RenderObjectStaticData& right) noexcept
	{
		return left.Mesh.RefersToSameResource(right.Mesh) && left.Material == right.Material &&
		       left.Skeleton == right.Skeleton && left.MeshKind == right.MeshKind &&
		       left.MeshAssetIndex == right.MeshAssetIndex && left.InstanceGroupIndex == right.InstanceGroupIndex;
	}
}
