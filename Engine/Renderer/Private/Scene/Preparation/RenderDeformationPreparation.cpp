#include "PCH.h"

#include "Scene/Preparation/RenderDeformationPreparation.h"

#include "Scene/RenderScene.h"
#include "Scene/Preparation/RenderPrimitivePreparation.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <algorithm>

void RenderDeformationPreparation::Prepare(
    const RenderScene& scene,
    std::span<ResolvedRenderPrimitive> primitives,
    RenderDeformationWork& work)
{
	ResetWork(work);
	ResetPrimitiveOutputs(primitives);
	PrepareJointMatrices(scene, scene.GetJointMatrixRanges(), scene.GetJointMatrices(), primitives, work);
	PrepareMorphWeights(scene, scene.GetMorphWeightRanges(), scene.GetMorphWeights(), primitives, work);
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

void RenderDeformationPreparation::ResetPrimitiveOutputs(std::span<ResolvedRenderPrimitive> primitives) noexcept
{
	for (ResolvedRenderPrimitive& primitive : primitives)
	{
		primitive.Draw.Skinning.JointMatrixOffset = kInvalidMeshInstanceJointMatrixOffset;
		primitive.Draw.Morph = {};
	}
}

void RenderDeformationPreparation::PrepareJointMatrices(
    const RenderScene& scene,
    std::span<const RenderJointMatrixRange> jointMatrixRanges,
    std::span<const DirectX::XMFLOAT4X4> jointMatrices,
    std::span<ResolvedRenderPrimitive> primitives,
    RenderDeformationWork& work)
{
	std::size_t jointMatrixCount = 0u;
	std::size_t primitiveIndex = 0u;
	for (const RenderJointMatrixRange& range : jointMatrixRanges)
	{
		while (primitiveIndex < primitives.size() && primitives[primitiveIndex].Object < range.Object)
		{
			++primitiveIndex;
		}
		if (primitiveIndex >= primitives.size() || primitives[primitiveIndex].Object != range.Object || !range.Skeleton.IsValid()
		    || !range.Animation.IsValid() || range.JointMatrixCount == 0 || range.JointMatrixOffset > jointMatrices.size()
		    || range.JointMatrixCount > jointMatrices.size() - range.JointMatrixOffset)
		{
			continue;
		}

		ResolvedRenderPrimitive& target = primitives[primitiveIndex];
		const std::span<const DirectX::XMFLOAT4X4> currentMatrices = jointMatrices.subspan(range.JointMatrixOffset, range.JointMatrixCount);
		const std::span<const DirectX::XMFLOAT4X4> history = scene.FindPreviousJointMatrices(range.Object);
		const std::span<const DirectX::XMFLOAT4X4> previousMatrices = history.size() == currentMatrices.size() ? history : currentMatrices;

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
    const RenderScene& scene,
    std::span<const RenderMorphWeightRange> morphWeightRanges,
    std::span<const float> weights,
    std::span<ResolvedRenderPrimitive> primitives,
    RenderDeformationWork& work)
{
	std::size_t morphWeightCount = 0u;
	std::size_t morphIndex = 0u;
	for (ResolvedRenderPrimitive& primitive : primitives)
	{
		if (primitive.MorphTargetCount == 0u)
		{
			continue;
		}

		while (morphIndex < morphWeightRanges.size() && morphWeightRanges[morphIndex].Object < primitive.Object)
		{
			++morphIndex;
		}

		const RenderMorphWeightRange* sample =
		    morphIndex < morphWeightRanges.size() && morphWeightRanges[morphIndex].Object == primitive.Object
		    ? &morphWeightRanges[morphIndex]
		    : nullptr;
		const std::span<const float> history = scene.FindPreviousMorphWeights(primitive.Object);
		if (sample == nullptr && (history.empty() || AreAllZero(history)))
		{
			continue;
		}

		const bool validSample =
		    sample != nullptr && sample->WeightOffset <= weights.size() && sample->WeightCount <= weights.size() - sample->WeightOffset;
		const std::span<const float> current =
		    validSample ? weights.subspan(sample->WeightOffset, sample->WeightCount) : std::span<const float>{};
		const std::span<const float> previous = history.size() == primitive.MorphTargetCount ? history : current;

		primitive.Draw.Morph = MeshDrawMorph{
		    .WeightOffset = static_cast<std::uint32_t>(morphWeightCount),
		    .TargetCount = primitive.MorphTargetCount,
		    .VertexCount = primitive.MorphTargetVertexCount};
		work.MorphWeightCopyRanges.push_back(
		    RenderMorphWeightCopyRange{
		        .Object = primitive.Object,
		        .OutputOffset = static_cast<std::uint32_t>(morphWeightCount),
		        .TargetCount = primitive.MorphTargetCount,
		        .Current = current,
		        .Previous = previous});
		morphWeightCount += primitive.MorphTargetCount;
	}

	work.MorphWeights.resize(morphWeightCount);
	work.PreviousMorphWeights.resize(morphWeightCount);
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
