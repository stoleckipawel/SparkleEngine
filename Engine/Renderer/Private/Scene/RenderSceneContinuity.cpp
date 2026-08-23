#include "PCH.h"
#include "Scene/RenderScene.h"

#include "Scene/Preparation/RenderDeformationPreparation.h"
#include "Scene/Preparation/RenderPrimitivePreparation.h"

#include <algorithm>

DirectX::XMFLOAT4X4 RenderScene::ResolvePreviousWorldMatrix(const RenderPrimitive& primitive) const noexcept
{
	if (primitive.GpuSceneSlot < m_previousWorldTransforms.size())
	{
		const PreviousWorldTransform& previous = m_previousWorldTransforms[primitive.GpuSceneSlot];
		if (previous.Object == primitive.Object)
		{
			return previous.WorldMatrix;
		}
	}
	return primitive.Dynamic.WorldMatrix;
}

std::span<const DirectX::XMFLOAT4X4> RenderScene::FindPreviousJointMatrices(RenderObjectId primitiveId) const noexcept
{
	const auto history = m_jointMatrixHistory.find(primitiveId);
	return history != m_jointMatrixHistory.end() ? std::span<const DirectX::XMFLOAT4X4>{history->second}
	                                             : std::span<const DirectX::XMFLOAT4X4>{};
}

std::span<const float> RenderScene::FindPreviousMorphWeights(RenderObjectId primitiveId) const noexcept
{
	const auto history = m_morphWeightHistory.find(primitiveId);
	return history != m_morphWeightHistory.end() ? std::span<const float>{history->second} : std::span<const float>{};
}

void RenderScene::CommitContinuity(std::span<const PreparedRenderPrimitive> primitives, const RenderDeformationWork& deformation)
{
	CommitPreviousWorldTransforms(primitives);
	CommitJointMatrixContinuity(deformation);
	CommitMorphWeightContinuity(deformation);
}

void RenderScene::CommitPreviousWorldTransforms(std::span<const PreparedRenderPrimitive> primitives)
{
	m_previousWorldTransforms.clear();

	std::uint32_t requiredSlotCount = 0u;
	for (const PreparedRenderPrimitive& primitive : primitives)
	{
		requiredSlotCount = (std::max) (requiredSlotCount, primitive.Draw.Source.GpuSceneSlot + 1u);
	}
	m_previousWorldTransforms.resize(requiredSlotCount);

	for (const PreparedRenderPrimitive& primitive : primitives)
	{
		if (primitive.Object.IsValid())
		{
			m_previousWorldTransforms[primitive.Draw.Source.GpuSceneSlot] =
			    PreviousWorldTransform{.Object = primitive.Object, .WorldMatrix = primitive.Draw.Transform.WorldMatrix};
		}
	}
}

void RenderScene::CommitJointMatrixContinuity(const RenderDeformationWork& deformation)
{
	std::size_t jointRangeIndex = 0u;
	for (auto history = m_jointMatrixHistory.begin(); history != m_jointMatrixHistory.end();)
	{
		while (jointRangeIndex < deformation.JointMatrixCopyRanges.size()
		    && deformation.JointMatrixCopyRanges[jointRangeIndex].Object < history->first)
		{
			++jointRangeIndex;
		}
		if (jointRangeIndex >= deformation.JointMatrixCopyRanges.size()
		    || deformation.JointMatrixCopyRanges[jointRangeIndex].Object != history->first)
		{
			history = m_jointMatrixHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderJointMatrixCopyRange& range : deformation.JointMatrixCopyRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.Current.size();
		if (end <= deformation.JointMatrices.size())
		{
			std::vector<DirectX::XMFLOAT4X4>& history = m_jointMatrixHistory[range.Object];
			history.assign(deformation.JointMatrices.begin() + begin, deformation.JointMatrices.begin() + end);
		}
	}
}

void RenderScene::CommitMorphWeightContinuity(const RenderDeformationWork& deformation)
{
	const auto retainsMorphHistory = [&deformation](const RenderMorphWeightCopyRange& range) noexcept
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.TargetCount;
		if (end > deformation.MorphWeights.size())
		{
			return false;
		}
		const std::span<const float> weights{deformation.MorphWeights.data() + begin, range.TargetCount};
		return !range.Current.empty() || !std::all_of(weights.begin(), weights.end(), [](float weight) { return weight == 0.0f; });
	};

	std::size_t morphRangeIndex = 0u;
	for (auto history = m_morphWeightHistory.begin(); history != m_morphWeightHistory.end();)
	{
		while (morphRangeIndex < deformation.MorphWeightCopyRanges.size()
		    && deformation.MorphWeightCopyRanges[morphRangeIndex].Object < history->first)
		{
			++morphRangeIndex;
		}
		const bool retain = morphRangeIndex < deformation.MorphWeightCopyRanges.size()
		    && deformation.MorphWeightCopyRanges[morphRangeIndex].Object == history->first
		    && retainsMorphHistory(deformation.MorphWeightCopyRanges[morphRangeIndex]);
		if (!retain)
		{
			history = m_morphWeightHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderMorphWeightCopyRange& range : deformation.MorphWeightCopyRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.TargetCount;
		if (end <= deformation.MorphWeights.size() && retainsMorphHistory(range))
		{
			std::vector<float>& history = m_morphWeightHistory[range.Object];
			history.assign(deformation.MorphWeights.begin() + begin, deformation.MorphWeights.begin() + end);
		}
	}
}

void RenderScene::ResetContinuity() noexcept
{
	m_previousWorldTransforms.clear();
	m_jointMatrixHistory.clear();
	m_morphWeightHistory.clear();
}
