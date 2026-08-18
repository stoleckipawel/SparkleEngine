#include "PCH.h"

#include "SceneData/Preparation/RenderDeformationPreparation.h"

#include "SceneData/Preparation/RenderObjectPreparation.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <algorithm>

void RenderDeformationPreparation::Prepare(
    const RenderSceneDynamicData& dynamic,
    std::span<ResolvedRenderObject> objects,
    RenderDeformationWork& work)
{
	ResetWork(work);
	ResetObjectOutputs(objects);
	PrepareJointMatrices(dynamic.JointMatrixRanges, dynamic.JointMatrices, objects, work);
	PrepareMorphWeights(dynamic.MorphWeightRanges, dynamic.MorphWeights, objects, work);
}

void RenderDeformationPreparation::ResetWork(RenderDeformationWork& work) noexcept
{
	work.JointMatrixCopyRanges.clear();
	work.MorphWeightCopyRanges.clear();
	work.JointMatrices.clear();
	work.PreviousJointMatrices.clear();
	work.MorphWeights.clear();
	work.PreviousMorphWeights.clear();
}

void RenderDeformationPreparation::ResetObjectOutputs(std::span<ResolvedRenderObject> objects) noexcept
{
	for (ResolvedRenderObject& object : objects)
	{
		object.Draw.Skinning.JointMatrixOffset = kInvalidMeshInstanceJointMatrixOffset;
		object.Draw.Morph = {};
	}
}

void RenderDeformationPreparation::PrepareJointMatrices(
    std::span<const RenderJointMatrixRange> jointMatrixRanges,
    std::span<const DirectX::XMFLOAT4X4> jointMatrices,
    std::span<ResolvedRenderObject> objects,
    RenderDeformationWork& work)
{
	std::size_t jointMatrixCount = 0u;
	std::size_t objectIndex = 0u;
	for (const RenderJointMatrixRange& range : jointMatrixRanges)
	{
		while (objectIndex < objects.size() && objects[objectIndex].Object < range.Object)
		{
			++objectIndex;
		}
		if (objectIndex >= objects.size() || objects[objectIndex].Object != range.Object || !range.Skeleton.IsValid()
		    || !range.Animation.IsValid() || range.JointMatrixCount == 0 || range.JointMatrixOffset > jointMatrices.size()
		    || range.JointMatrixCount > jointMatrices.size() - range.JointMatrixOffset)
		{
			continue;
		}

		ResolvedRenderObject& target = objects[objectIndex];
		const std::span<const DirectX::XMFLOAT4X4> currentMatrices = jointMatrices.subspan(range.JointMatrixOffset, range.JointMatrixCount);
		const auto previous = m_jointMatrixHistory.find(range.Object);
		const std::span<const DirectX::XMFLOAT4X4> previousMatrices =
		    previous != m_jointMatrixHistory.end() && previous->second.size() == currentMatrices.size()
		    ? std::span<const DirectX::XMFLOAT4X4>{previous->second}
		    : currentMatrices;

		target.Draw.Skinning.JointMatrixOffset = static_cast<std::uint32_t>(jointMatrixCount);
		work.JointMatrixCopyRanges.push_back(
		    RenderJointMatrixCopyRange{
		        .Object = range.Object,
		        .OutputOffset = static_cast<std::uint32_t>(jointMatrixCount),
		        .Current = currentMatrices,
		        .Previous = previousMatrices});
		jointMatrixCount += currentMatrices.size();
	}

	work.JointMatrices.resize(jointMatrixCount);
	work.PreviousJointMatrices.resize(jointMatrixCount);
}

void RenderDeformationPreparation::PrepareMorphWeights(
    std::span<const RenderMorphWeightRange> morphWeightRanges,
    std::span<const float> weights,
    std::span<ResolvedRenderObject> objects,
    RenderDeformationWork& work)
{
	std::size_t morphWeightCount = 0u;
	std::size_t morphIndex = 0u;
	for (ResolvedRenderObject& object : objects)
	{
		if (object.MorphTargetCount == 0u)
		{
			continue;
		}

		while (morphIndex < morphWeightRanges.size() && morphWeightRanges[morphIndex].Object < object.Object)
		{
			++morphIndex;
		}

		const RenderMorphWeightRange* sample =
		    morphIndex < morphWeightRanges.size() && morphWeightRanges[morphIndex].Object == object.Object ? &morphWeightRanges[morphIndex]
		                                                                                                   : nullptr;
		const auto history = m_morphWeightHistory.find(object.Object);
		if (sample == nullptr && (history == m_morphWeightHistory.end() || AreAllZero(history->second)))
		{
			continue;
		}

		const bool validSample =
		    sample != nullptr && sample->WeightOffset <= weights.size() && sample->WeightCount <= weights.size() - sample->WeightOffset;
		const std::span<const float> current =
		    validSample ? weights.subspan(sample->WeightOffset, sample->WeightCount) : std::span<const float>{};
		const std::span<const float> previous = history != m_morphWeightHistory.end() && history->second.size() == object.MorphTargetCount
		    ? std::span<const float>{history->second}
		    : current;

		object.Draw.Morph = MeshDrawMorph{
		    .WeightOffset = static_cast<std::uint32_t>(morphWeightCount),
		    .TargetCount = object.MorphTargetCount,
		    .VertexCount = object.MorphTargetVertexCount};
		work.MorphWeightCopyRanges.push_back(
		    RenderMorphWeightCopyRange{
		        .Object = object.Object,
		        .OutputOffset = static_cast<std::uint32_t>(morphWeightCount),
		        .TargetCount = object.MorphTargetCount,
		        .Current = current,
		        .Previous = previous});
		morphWeightCount += object.MorphTargetCount;
	}

	work.MorphWeights.resize(morphWeightCount);
	work.PreviousMorphWeights.resize(morphWeightCount);
}

void RenderDeformationPreparation::Commit(const RenderDeformationWork& work)
{
	CommitJointMatrixHistory(work);
	CommitMorphWeightHistory(work);
}

void RenderDeformationPreparation::CommitJointMatrixHistory(const RenderDeformationWork& work)
{
	std::size_t rangeIndex = 0u;
	for (auto history = m_jointMatrixHistory.begin(); history != m_jointMatrixHistory.end();)
	{
		while (rangeIndex < work.JointMatrixCopyRanges.size() && work.JointMatrixCopyRanges[rangeIndex].Object < history->first)
		{
			++rangeIndex;
		}
		if (rangeIndex >= work.JointMatrixCopyRanges.size() || work.JointMatrixCopyRanges[rangeIndex].Object != history->first)
		{
			history = m_jointMatrixHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderJointMatrixCopyRange& range : work.JointMatrixCopyRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.Current.size();
		if (end <= work.JointMatrices.size())
		{
			std::vector<DirectX::XMFLOAT4X4>& history = m_jointMatrixHistory[range.Object];
			history.assign(work.JointMatrices.begin() + begin, work.JointMatrices.begin() + end);
		}
	}
}

void RenderDeformationPreparation::CommitMorphWeightHistory(const RenderDeformationWork& work)
{
	std::size_t rangeIndex = 0u;
	for (auto history = m_morphWeightHistory.begin(); history != m_morphWeightHistory.end();)
	{
		while (rangeIndex < work.MorphWeightCopyRanges.size() && work.MorphWeightCopyRanges[rangeIndex].Object < history->first)
		{
			++rangeIndex;
		}
		const bool retain = rangeIndex < work.MorphWeightCopyRanges.size()
		    && work.MorphWeightCopyRanges[rangeIndex].Object == history->first
		    && RetainsMorphWeightHistory(work, work.MorphWeightCopyRanges[rangeIndex]);
		if (!retain)
		{
			history = m_morphWeightHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderMorphWeightCopyRange& range : work.MorphWeightCopyRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.TargetCount;
		if (end > work.MorphWeights.size() || !RetainsMorphWeightHistory(work, range))
		{
			continue;
		}
		std::vector<float>& history = m_morphWeightHistory[range.Object];
		history.assign(work.MorphWeights.begin() + begin, work.MorphWeights.begin() + end);
	}
}

bool RenderDeformationPreparation::RetainsMorphWeightHistory(
    const RenderDeformationWork& work,
    const RenderMorphWeightCopyRange& range) noexcept
{
	const std::size_t begin = range.OutputOffset;
	const std::size_t end = begin + range.TargetCount;
	if (end > work.MorphWeights.size())
	{
		return false;
	}
	return !range.Current.empty() || !AreAllZero(std::span<const float>{work.MorphWeights.data() + begin, range.TargetCount});
}

void RenderDeformationPreparation::Reset() noexcept
{
	m_jointMatrixHistory.clear();
	m_morphWeightHistory.clear();
}

void RenderDeformationPreparation::CopyJointMatrixRanges(
    std::span<const RenderJointMatrixCopyRange> ranges,
    std::span<DirectX::XMFLOAT4X4> current,
    std::span<DirectX::XMFLOAT4X4> previous) noexcept
{
	for (const RenderJointMatrixCopyRange& range : ranges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t count = range.Current.size();
		if (begin > current.size() || count > current.size() - begin || begin > previous.size() || count > previous.size() - begin)
		{
			continue;
		}
		std::copy(range.Current.begin(), range.Current.end(), current.begin() + begin);
		std::copy(range.Previous.begin(), range.Previous.end(), previous.begin() + begin);
	}
}

void RenderDeformationPreparation::CopyMorphWeightRanges(
    std::span<const RenderMorphWeightCopyRange> ranges,
    std::span<float> current,
    std::span<float> previous) noexcept
{
	for (const RenderMorphWeightCopyRange& range : ranges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t count = range.TargetCount;
		if (begin > current.size() || count > current.size() - begin || begin > previous.size() || count > previous.size() - begin)
		{
			continue;
		}
		CopyMorphWeightSpan(range.Current, current.subspan(begin, count));
		CopyMorphWeightSpan(range.Previous.empty() ? range.Current : range.Previous, previous.subspan(begin, count));
	}
}

bool RenderDeformationPreparation::AreAllZero(std::span<const float> weights) noexcept
{
	return std::all_of(weights.begin(), weights.end(), [](float weight) { return weight == 0.0f; });
}

void RenderDeformationPreparation::CopyMorphWeightSpan(std::span<const float> source, std::span<float> destination) noexcept
{
	std::fill(destination.begin(), destination.end(), 0.0f);
	const std::size_t count = (std::min) (source.size(), destination.size());
	std::copy_n(source.begin(), count, destination.begin());
}
