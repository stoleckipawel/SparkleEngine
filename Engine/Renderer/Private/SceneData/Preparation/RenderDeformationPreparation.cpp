#include "PCH.h"

#include "SceneData/Preparation/RenderDeformationPreparation.h"

#include "SceneData/Preparation/RenderObjectPreparation.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <algorithm>

void RenderDeformationPreparation::Prepare(
    const RenderFrameDynamicData& dynamic,
    std::span<ResolvedRenderObject> objects,
    RenderDeformationWork& work)
{
	ResetWork(work);
	ResetObjectOutputs(objects);
	PrepareSkinning(dynamic.Skinning, objects, work);
	PrepareMorph(dynamic.MorphWeights, objects, work);
}

void RenderDeformationPreparation::ResetWork(RenderDeformationWork& work) noexcept
{
	work.SkinningRanges.clear();
	work.MorphRanges.clear();
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

void RenderDeformationPreparation::PrepareSkinning(
    std::span<const RenderSkinningData> skinningRows,
    std::span<ResolvedRenderObject> objects,
    RenderDeformationWork& work)
{
	std::size_t jointMatrixCount = 0u;
	std::size_t objectIndex = 0u;
	for (const RenderSkinningData& skinning : skinningRows)
	{
		while (objectIndex < objects.size() &&
		       objects[objectIndex].Object < skinning.Object)
		{
			++objectIndex;
		}
		if (objectIndex >= objects.size() ||
		    objects[objectIndex].Object != skinning.Object ||
		    !skinning.Skeleton.IsValid() ||
		    !skinning.Animation.IsValid() ||
		    skinning.Matrices.empty())
		{
			continue;
		}

		ResolvedRenderObject& target =
		    objects[objectIndex];
		const auto previous = m_skinningHistory.find(skinning.Object);
		const std::span<const DirectX::XMFLOAT4X4> previousMatrices =
		    previous != m_skinningHistory.end() && previous->second.size() == skinning.Matrices.size()
		        ? std::span<const DirectX::XMFLOAT4X4>{previous->second}
		        : std::span<const DirectX::XMFLOAT4X4>{skinning.Matrices};

		target.Draw.Skinning.JointMatrixOffset = static_cast<std::uint32_t>(jointMatrixCount);
		work.SkinningRanges.push_back(
		    RenderSkinningCopyRange{
		        .Object = skinning.Object,
		        .OutputOffset = static_cast<std::uint32_t>(jointMatrixCount),
		        .Current = skinning.Matrices,
		        .Previous = previousMatrices});
		jointMatrixCount += skinning.Matrices.size();
	}

	work.JointMatrices.resize(jointMatrixCount);
	work.PreviousJointMatrices.resize(jointMatrixCount);
}

void RenderDeformationPreparation::PrepareMorph(
    std::span<const RenderMorphData> morphWeights,
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

		while (morphIndex < morphWeights.size() &&
		       morphWeights[morphIndex].Object < object.Object)
		{
			++morphIndex;
		}

		const RenderMorphData* sample =
		    morphIndex < morphWeights.size() && morphWeights[morphIndex].Object == object.Object
		        ? &morphWeights[morphIndex]
		        : nullptr;
		const auto history = m_morphHistory.find(object.Object);
		if (sample == nullptr &&
		    (history == m_morphHistory.end() ||
		     AreAllZero(history->second)))
		{
			continue;
		}

		const std::span<const float> current =
		    sample != nullptr ? std::span<const float>{sample->Weights} : std::span<const float>{};
		const std::span<const float> previous =
		    history != m_morphHistory.end() && history->second.size() == object.MorphTargetCount
		        ? std::span<const float>{history->second}
		        : current;

		object.Draw.Morph = MeshDrawMorph{
		    .WeightOffset = static_cast<std::uint32_t>(morphWeightCount),
		    .TargetCount = object.MorphTargetCount,
		    .VertexCount = object.MorphTargetVertexCount};
		work.MorphRanges.push_back(
		    RenderMorphCopyRange{
		        .Object = object.Object,
		        .OutputOffset = static_cast<std::uint32_t>(morphWeightCount),
		        .TargetCount =
		            object.MorphTargetCount,
		        .Current = current,
		        .Previous = previous});
		morphWeightCount += object.MorphTargetCount;
	}

	work.MorphWeights.resize(morphWeightCount);
	work.PreviousMorphWeights.resize(morphWeightCount);
}

void RenderDeformationPreparation::Commit(
    const RenderDeformationWork& work)
{
	CommitSkinningHistory(work);
	CommitMorphHistory(work);
}

void RenderDeformationPreparation::CommitSkinningHistory(
    const RenderDeformationWork& work)
{
	std::size_t rangeIndex = 0u;
	for (auto history = m_skinningHistory.begin();
	     history != m_skinningHistory.end();)
	{
		while (rangeIndex < work.SkinningRanges.size() &&
		       work.SkinningRanges[rangeIndex].Object <
		           history->first)
		{
			++rangeIndex;
		}
		if (rangeIndex >= work.SkinningRanges.size() ||
		    work.SkinningRanges[rangeIndex].Object !=
		        history->first)
		{
			history = m_skinningHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderSkinningCopyRange& range :
	     work.SkinningRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end =
		    begin + range.Current.size();
		if (end <= work.JointMatrices.size())
		{
			std::vector<DirectX::XMFLOAT4X4>& history =
			    m_skinningHistory[range.Object];
			history.assign(
			    work.JointMatrices.begin() + begin,
			    work.JointMatrices.begin() + end);
		}
	}
}

void RenderDeformationPreparation::CommitMorphHistory(
    const RenderDeformationWork& work)
{
	std::size_t rangeIndex = 0u;
	for (auto history = m_morphHistory.begin();
	     history != m_morphHistory.end();)
	{
		while (rangeIndex < work.MorphRanges.size() &&
		       work.MorphRanges[rangeIndex].Object <
		           history->first)
		{
			++rangeIndex;
		}
		const bool retain =
		    rangeIndex < work.MorphRanges.size() &&
		    work.MorphRanges[rangeIndex].Object ==
		        history->first &&
		    RetainsMorphHistory(
		        work,
		        work.MorphRanges[rangeIndex]);
		if (!retain)
		{
			history = m_morphHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderMorphCopyRange& range :
	     work.MorphRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end =
		    begin + range.TargetCount;
		if (end > work.MorphWeights.size() ||
		    !RetainsMorphHistory(work, range))
		{
			continue;
		}
		std::vector<float>& history =
		    m_morphHistory[range.Object];
		history.assign(
		    work.MorphWeights.begin() + begin,
		    work.MorphWeights.begin() + end);
	}
}

bool RenderDeformationPreparation::RetainsMorphHistory(
    const RenderDeformationWork& work,
    const RenderMorphCopyRange& range) noexcept
{
	const std::size_t begin = range.OutputOffset;
	const std::size_t end = begin + range.TargetCount;
	if (end > work.MorphWeights.size())
	{
		return false;
	}
	return !range.Current.empty() ||
	       !AreAllZero(
	           std::span<const float>{
	               work.MorphWeights.data() + begin,
	               range.TargetCount});
}

void RenderDeformationPreparation::Reset() noexcept
{
	m_skinningHistory.clear();
	m_morphHistory.clear();
}

void RenderDeformationPreparation::CopySkinningRanges(
    std::span<const RenderSkinningCopyRange> ranges,
    std::span<DirectX::XMFLOAT4X4> current,
    std::span<DirectX::XMFLOAT4X4> previous) noexcept
{
	for (const RenderSkinningCopyRange& range : ranges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t count = range.Current.size();
		if (begin > current.size() ||
		    count > current.size() - begin ||
		    begin > previous.size() ||
		    count > previous.size() - begin)
		{
			continue;
		}
		std::copy(
		    range.Current.begin(),
		    range.Current.end(),
		    current.begin() + begin);
		std::copy(
		    range.Previous.begin(),
		    range.Previous.end(),
		    previous.begin() + begin);
	}
}

void RenderDeformationPreparation::CopyMorphRanges(
    std::span<const RenderMorphCopyRange> ranges,
    std::span<float> current,
    std::span<float> previous) noexcept
{
	for (const RenderMorphCopyRange& range : ranges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t count = range.TargetCount;
		if (begin > current.size() ||
		    count > current.size() - begin ||
		    begin > previous.size() ||
		    count > previous.size() - begin)
		{
			continue;
		}
		CopyMorphWeights(
		    range.Current,
		    current.subspan(begin, count));
		CopyMorphWeights(
		    range.Previous.empty()
		        ? range.Current
		        : range.Previous,
		    previous.subspan(begin, count));
	}
}

bool RenderDeformationPreparation::AreAllZero(
    std::span<const float> weights) noexcept
{
	return std::all_of(
	    weights.begin(),
	    weights.end(),
	    [](float weight)
	    {
		    return weight == 0.0f;
	    });
}

void RenderDeformationPreparation::CopyMorphWeights(
    std::span<const float> source,
    std::span<float> destination) noexcept
{
	std::fill(
	    destination.begin(),
	    destination.end(),
	    0.0f);
	const std::size_t count =
	    (std::min)(source.size(), destination.size());
	std::copy_n(
	    source.begin(),
	    count,
	    destination.begin());
}
