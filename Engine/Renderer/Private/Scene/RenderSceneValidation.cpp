#include "PCH.h"
#include "Scene/RenderScene.h"

#include <algorithm>

bool RenderScene::ValidateDynamic(const RenderSceneDynamicData& dynamic, const RenderSceneDelta& delta) const
{
	if (!HasStrictlyOrderedDynamicObjects(dynamic.Objects) || !HasStrictlyOrderedJointMatrixRanges(dynamic.JointMatrixRanges)
	    || !HasStrictlyOrderedMorphWeightRanges(dynamic.MorphWeightRanges))
	{
		return false;
	}

	for (const RenderObjectDynamicData& primitive : dynamic.Objects)
	{
		if (!primitive.Object.IsValid() || !IsObjectAvailable(primitive.Object, delta))
		{
			return false;
		}
	}

	for (const RenderObjectCreate& create : delta.Creates)
	{
		const auto dynamicObject = std::lower_bound(
		    dynamic.Objects.begin(),
		    dynamic.Objects.end(),
		    create.Object,
		    [](const RenderObjectDynamicData& primitive, RenderObjectId identity) { return primitive.Object < identity; });
		const bool hasDynamicData = dynamicObject != dynamic.Objects.end() && dynamicObject->Object == create.Object;
		if (!hasDynamicData)
		{
			return false;
		}
	}

	for (const RenderJointMatrixRange& range : dynamic.JointMatrixRanges)
	{
		if (!range.Object.IsValid() || !IsObjectAvailable(range.Object, delta) || range.JointMatrixOffset > dynamic.JointMatrices.size()
		    || range.JointMatrixCount > dynamic.JointMatrices.size() - range.JointMatrixOffset)
		{
			return false;
		}
	}

	for (const RenderMorphWeightRange& morphWeightRange : dynamic.MorphWeightRanges)
	{
		if (!morphWeightRange.Object.IsValid() || !IsObjectAvailable(morphWeightRange.Object, delta)
		    || morphWeightRange.WeightOffset > dynamic.MorphWeights.size()
		    || morphWeightRange.WeightCount > dynamic.MorphWeights.size() - morphWeightRange.WeightOffset)
		{
			return false;
		}
	}
	return true;
}

bool RenderScene::ValidateDelta(const RenderSceneDelta& delta) const
{
	if (delta.SceneGeneration == 0 || delta.SequenceNumber == 0)
	{
		return false;
	}
	if (delta.SceneGeneration < m_sceneGeneration)
	{
		return false;
	}
	if (delta.SceneGeneration == m_sceneGeneration && delta.SequenceNumber == m_sequenceNumber)
	{
		return false;
	}
	if (delta.SceneGeneration == m_sceneGeneration && m_sequenceNumber != 0 && delta.SequenceNumber != m_sequenceNumber + 1)
	{
		return false;
	}
	if (delta.SceneGeneration > m_sceneGeneration && !delta.ResetScene)
	{
		return false;
	}
	if (!HasOrderedDeltaObjects(delta))
	{
		return false;
	}
	if (HasConflictingDeltaObjects(delta))
	{
		return false;
	}

	for (RenderObjectId primitiveId : delta.Destroys)
	{
		if (!primitiveId.IsValid() || delta.ResetScene || Find(primitiveId) == nullptr)
		{
			return false;
		}
	}
	for (const RenderObjectCreate& create : delta.Creates)
	{
		if (!create.Object.IsValid() || !create.Static.Mesh.IsValid() || (!delta.ResetScene && Find(create.Object) != nullptr))
		{
			return false;
		}
	}
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if (!update.Object.IsValid() || !update.Static.Mesh.IsValid() || delta.ResetScene || Find(update.Object) == nullptr)
		{
			return false;
		}
	}
	return true;
}

bool RenderScene::HasOrderedDeltaObjects(const RenderSceneDelta& delta) noexcept
{
	const bool createsOrdered = std::is_sorted(
	    delta.Creates.begin(),
	    delta.Creates.end(),
	    [](const RenderObjectCreate& left, const RenderObjectCreate& right) { return left.Object < right.Object; });
	const bool updatesOrdered = std::is_sorted(
	    delta.Updates.begin(),
	    delta.Updates.end(),
	    [](const RenderObjectUpdate& left, const RenderObjectUpdate& right) { return left.Object < right.Object; });
	return createsOrdered && updatesOrdered && std::is_sorted(delta.Destroys.begin(), delta.Destroys.end());
}

bool RenderScene::HasConflictingDeltaObjects(const RenderSceneDelta& delta) noexcept
{
	const auto duplicateCreates = std::adjacent_find(
	    delta.Creates.begin(),
	    delta.Creates.end(),
	    [](const RenderObjectCreate& left, const RenderObjectCreate& right) { return left.Object == right.Object; });
	const auto duplicateUpdates = std::adjacent_find(
	    delta.Updates.begin(),
	    delta.Updates.end(),
	    [](const RenderObjectUpdate& left, const RenderObjectUpdate& right) { return left.Object == right.Object; });
	const auto duplicateDestroys = std::adjacent_find(delta.Destroys.begin(), delta.Destroys.end());
	if (duplicateCreates != delta.Creates.end() || duplicateUpdates != delta.Updates.end() || duplicateDestroys != delta.Destroys.end())
	{
		return true;
	}

	for (const RenderObjectCreate& create : delta.Creates)
	{
		const auto update = std::lower_bound(
		    delta.Updates.begin(),
		    delta.Updates.end(),
		    create.Object,
		    [](const RenderObjectUpdate& candidate, RenderObjectId identity) { return candidate.Object < identity; });
		if ((update != delta.Updates.end() && update->Object == create.Object)
		    || std::binary_search(delta.Destroys.begin(), delta.Destroys.end(), create.Object))
		{
			return true;
		}
	}

	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if (std::binary_search(delta.Destroys.begin(), delta.Destroys.end(), update.Object))
		{
			return true;
		}
	}
	return false;
}

bool RenderScene::HasStrictlyOrderedDynamicObjects(std::span<const RenderObjectDynamicData> primitives) noexcept
{
	for (std::size_t index = 1u; index < primitives.size(); ++index)
	{
		if (!(primitives[index - 1u].Object < primitives[index].Object))
		{
			return false;
		}
	}
	return true;
}

bool RenderScene::HasStrictlyOrderedJointMatrixRanges(std::span<const RenderJointMatrixRange> ranges) noexcept
{
	for (std::size_t index = 1u; index < ranges.size(); ++index)
	{
		if (!(ranges[index - 1u].Object < ranges[index].Object))
		{
			return false;
		}
	}
	return true;
}

bool RenderScene::HasStrictlyOrderedMorphWeightRanges(std::span<const RenderMorphWeightRange> ranges) noexcept
{
	for (std::size_t index = 1u; index < ranges.size(); ++index)
	{
		if (!(ranges[index - 1u].Object < ranges[index].Object))
		{
			return false;
		}
	}
	return true;
}
