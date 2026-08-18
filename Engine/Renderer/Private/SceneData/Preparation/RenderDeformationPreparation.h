#pragma once

#include "Rendering/RenderSceneDynamicData.h"
#include "Rendering/RenderObjectId.h"

#include <DirectXMath.h>

#include <cstdint>
#include <map>
#include <span>
#include <vector>

struct ResolvedRenderObject;

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
	void Prepare(const RenderSceneDynamicData& dynamic, std::span<ResolvedRenderObject> objects, RenderDeformationWork& work);
	void Commit(const RenderDeformationWork& work);
	void Reset() noexcept;

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
	    std::span<const RenderJointMatrixRange> jointMatrixRanges,
	    std::span<const DirectX::XMFLOAT4X4> jointMatrices,
	    std::span<ResolvedRenderObject> objects,
	    RenderDeformationWork& work);
	void PrepareMorphWeights(
	    std::span<const RenderMorphWeightRange> morphWeightRanges,
	    std::span<const float> weights,
	    std::span<ResolvedRenderObject> objects,
	    RenderDeformationWork& work);
	static bool AreAllZero(std::span<const float> weights) noexcept;
	static void CopyMorphWeightSpan(std::span<const float> source, std::span<float> destination) noexcept;
	void CommitJointMatrixHistory(const RenderDeformationWork& work);
	void CommitMorphWeightHistory(const RenderDeformationWork& work);
	static bool RetainsMorphWeightHistory(const RenderDeformationWork& work, const RenderMorphWeightCopyRange& range) noexcept;

	std::map<RenderObjectId, std::vector<DirectX::XMFLOAT4X4>> m_jointMatrixHistory;
	std::map<RenderObjectId, std::vector<float>> m_morphWeightHistory;
};
