#include "PCH.h"
#include "World/Extraction/Structural/RenderObjectDeltaExtractor.h"

#include "World/Extraction/Identity/RenderObjectIdentityMap.h"
#include "World/Extraction/WorldExtractionStorage.h"

#include <algorithm>

namespace ECS
{
	void RenderObjectDeltaExtractor::BeginScene() noexcept
	{
		m_published.clear();
		m_current.clear();
	}

	void RenderObjectDeltaExtractor::Extract(
	    std::span<const WorldExtractionStorage::MeshSlot> meshes,
	    RenderObjectIdentityMap& identities,
	    RenderWorldDelta& delta)
	{
		m_current.clear();
		m_current.reserve(meshes.size());

		std::size_t publishedIndex = 0u;
		for (const WorldExtractionStorage::MeshSlot& mesh : meshes)
		{
			RetirePublishedBefore(mesh.Entity, publishedIndex, delta);
			m_current.push_back(ResolveObject(mesh, identities, publishedIndex, delta));
		}

		RetireRemaining(publishedIndex, delta);
		SortDeltaObjects(delta);
		m_published.swap(m_current);
	}

	void RenderObjectDeltaExtractor::RetirePublishedBefore(
	    EntityId entity,
	    std::size_t& publishedIndex,
	    RenderWorldDelta& delta) const
	{
		while (publishedIndex < m_published.size() &&
		       m_published[publishedIndex].Entity < entity)
		{
			delta.Destroys.push_back(m_published[publishedIndex].Object);
			++publishedIndex;
		}
	}

	RenderObjectDeltaExtractor::PublishedObject RenderObjectDeltaExtractor::ResolveObject(
	    const WorldExtractionStorage::MeshSlot& mesh,
	    RenderObjectIdentityMap& identities,
	    std::size_t& publishedIndex,
	    RenderWorldDelta& delta) const
	{
		const RenderObjectStaticData staticData{
		    .Mesh = mesh.Mesh,
		    .Material = mesh.Material,
		    .Skeleton = mesh.Skeleton,
		    .MeshKind = mesh.Kind,
		    .MeshAssetIndex = mesh.MeshAssetIndex,
		    .InstanceGroupIndex = mesh.InstanceGroupIndex};

		if (publishedIndex >= m_published.size() ||
		    m_published[publishedIndex].Entity != mesh.Entity)
		{
			PublishedObject published{
			    .Entity = mesh.Entity,
			    .Object = identities.Resolve(mesh.Entity),
			    .Static = staticData};
			delta.Creates.push_back({published.Object, published.Static});
			return published;
		}

		PublishedObject published = m_published[publishedIndex++];
		if (!HasSameStaticData(published.Static, staticData))
		{
			published.Static = staticData;
			delta.Updates.push_back({published.Object, published.Static});
		}
		return published;
	}

	void RenderObjectDeltaExtractor::RetireRemaining(
	    std::size_t publishedIndex,
	    RenderWorldDelta& delta) const
	{
		for (; publishedIndex < m_published.size(); ++publishedIndex)
		{
			delta.Destroys.push_back(m_published[publishedIndex].Object);
		}
	}

	void RenderObjectDeltaExtractor::SortDeltaObjects(RenderWorldDelta& delta)
	{
		std::sort(
		    delta.Creates.begin(),
		    delta.Creates.end(),
		    [](const RenderObjectCreate& left, const RenderObjectCreate& right)
		    {
			    return left.Object < right.Object;
		    });
		std::sort(
		    delta.Updates.begin(),
		    delta.Updates.end(),
		    [](const RenderObjectUpdate& left, const RenderObjectUpdate& right)
		    {
			    return left.Object < right.Object;
		    });
		std::sort(delta.Destroys.begin(), delta.Destroys.end());
	}

	RenderObjectId RenderObjectDeltaExtractor::FindObject(EntityId entity) const noexcept
	{
		const auto object = std::lower_bound(
		    m_published.begin(),
		    m_published.end(),
		    entity,
		    [](const PublishedObject& candidate, EntityId identity)
		    {
			    return candidate.Entity < identity;
		    });
		return object == m_published.end() || object->Entity != entity
		           ? RenderObjectId{}
		           : object->Object;
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
