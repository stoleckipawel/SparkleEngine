#pragma once

#include "Rendering/RenderObjectId.h"
#include "Rendering/RenderSceneDynamicData.h"

#include <DirectXMath.h>

#include <cstdint>
#include <span>
#include <vector>

struct ResolvedRenderObject;
class RenderScene;

struct RenderJointMatrixCopyRange final
{
	RenderObjectId Object;
	std::uint32_t OutputOffset = 0u;
	std::span<const DirectX::XMFLOAT4X4> Current;
	std::span<const DirectX::XMFLOAT4X4> Previous;
};

struct RenderMorphWeightCopyRange final
{
	RenderObjectId Object;
	std::uint32_t OutputOffset = 0u;
	std::uint32_t TargetCount = 0u;
	std::span<const float> Current;
	std::span<const float> Previous;
};

struct RenderDeformationWork final
{
	std::vector<RenderJointMatrixCopyRange> JointMatrixCopyRanges;
	std::vector<RenderMorphWeightCopyRange> MorphWeightCopyRanges;
	std::vector<DirectX::XMFLOAT4X4> JointMatrices;
	std::vector<DirectX::XMFLOAT4X4> PreviousJointMatrices;
	std::vector<float> MorphWeights;
	std::vector<float> PreviousMorphWeights;
};

class RenderDeformationPreparation final
{
public:
	void Prepare(const RenderScene& scene, std::span<ResolvedRenderObject> objects, RenderDeformationWork& work);

	static void CopyJointMatrixRanges(
	    std::span<const RenderJointMatrixCopyRange> ranges,
	    std::span<DirectX::XMFLOAT4X4> current,
	    std::span<DirectX::XMFLOAT4X4> previous) noexcept;
	static void CopyMorphWeightRanges(
	    std::span<const RenderMorphWeightCopyRange> ranges,
	    std::span<float> current,
	    std::span<float> previous) noexcept;

private:
	static void ResetWork(RenderDeformationWork& work) noexcept;
	static void ResetObjectOutputs(std::span<ResolvedRenderObject> objects) noexcept;
	void PrepareJointMatrices(
	    const RenderScene& scene,
	    std::span<const RenderJointMatrixRange> jointMatrixRanges,
	    std::span<const DirectX::XMFLOAT4X4> jointMatrices,
	    std::span<ResolvedRenderObject> objects,
	    RenderDeformationWork& work);
	void PrepareMorphWeights(
	    const RenderScene& scene,
	    std::span<const RenderMorphWeightRange> morphWeightRanges,
	    std::span<const float> weights,
	    std::span<ResolvedRenderObject> objects,
	    RenderDeformationWork& work);
	static bool AreAllZero(std::span<const float> weights) noexcept;
	static void CopyMorphWeightSpan(std::span<const float> source, std::span<float> destination) noexcept;
};
