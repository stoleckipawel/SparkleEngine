#pragma once

#include "Rendering/RenderFrameDynamicData.h"
#include "Rendering/RenderObjectId.h"

#include <DirectXMath.h>

#include <cstdint>
#include <map>
#include <span>
#include <vector>

struct ResolvedRenderObject;

struct RenderSkinningCopyRange final
{
	RenderObjectId Object;
	std::uint32_t OutputOffset = 0u;
	std::span<const DirectX::XMFLOAT4X4> Current;
	std::span<const DirectX::XMFLOAT4X4> Previous;
};

struct RenderMorphCopyRange final
{
	RenderObjectId Object;
	std::uint32_t OutputOffset = 0u;
	std::uint32_t TargetCount = 0u;
	std::span<const float> Current;
	std::span<const float> Previous;
};

struct RenderDeformationWork final
{
	std::vector<RenderSkinningCopyRange> SkinningRanges;
	std::vector<RenderMorphCopyRange> MorphRanges;
	std::vector<DirectX::XMFLOAT4X4> JointMatrices;
	std::vector<DirectX::XMFLOAT4X4> PreviousJointMatrices;
	std::vector<float> MorphWeights;
	std::vector<float> PreviousMorphWeights;
};

class RenderDeformationPreparation final
{
  public:
	void Prepare(
	    const RenderFrameDynamicData& dynamic,
	    std::span<ResolvedRenderObject> objects,
	    RenderDeformationWork& work);
	void Commit(const RenderDeformationWork& work);
	void Reset() noexcept;

	static void CopySkinningRanges(
	    std::span<const RenderSkinningCopyRange> ranges,
	    std::span<DirectX::XMFLOAT4X4> current,
	    std::span<DirectX::XMFLOAT4X4> previous) noexcept;
	static void CopyMorphRanges(
	    std::span<const RenderMorphCopyRange> ranges,
	    std::span<float> current,
	    std::span<float> previous) noexcept;

  private:
	static void ResetWork(RenderDeformationWork& work) noexcept;
	static void ResetObjectOutputs(std::span<ResolvedRenderObject> objects) noexcept;
	void PrepareSkinning(
	    std::span<const RenderSkinningData> skinning,
	    std::span<const DirectX::XMFLOAT4X4> matrices,
	    std::span<ResolvedRenderObject> objects,
	    RenderDeformationWork& work);
	void PrepareMorph(
	    std::span<const RenderMorphData> morphWeights,
	    std::span<const float> weights,
	    std::span<ResolvedRenderObject> objects,
	    RenderDeformationWork& work);
	static bool AreAllZero(
	    std::span<const float> weights) noexcept;
	static void CopyMorphWeights(
	    std::span<const float> source,
	    std::span<float> destination) noexcept;
	void CommitSkinningHistory(
	    const RenderDeformationWork& work);
	void CommitMorphHistory(
	    const RenderDeformationWork& work);
	static bool RetainsMorphHistory(
	    const RenderDeformationWork& work,
	    const RenderMorphCopyRange& range) noexcept;

	std::map<
	    RenderObjectId,
	    std::vector<DirectX::XMFLOAT4X4>>
	    m_skinningHistory;
	std::map<RenderObjectId, std::vector<float>>
	    m_morphHistory;
};
