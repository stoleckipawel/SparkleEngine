#include "PCH.h"
#include "SceneData/GpuScene/RenderGpuGeometryState.h"

#include "Core/Public/Math/MathUtils.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <cstring>

void RenderGpuGeometryState::Update(const RenderSceneData& sceneData)
{
	UpdateMeshInstances(sceneData);
	UpdateMeshInstanceSlots(sceneData);
	UpdateDeformation(sceneData);
}

void RenderGpuGeometryState::CollectMeshInstanceWriteRanges(
    std::uint64_t appliedRevision,
    std::vector<StructuredBufferElementRange>& ranges) const
{
	ranges.clear();
	if (appliedRevision >= m_meshInstanceRevision)
	{
		return;
	}

	std::size_t index = 0u;
	while (index < m_meshInstanceElementRevisions.size())
	{
		if (m_meshInstanceElementRevisions[index] <= appliedRevision)
		{
			++index;
			continue;
		}

		const std::size_t firstElement = index;
		do
		{
			++index;
		} while (index < m_meshInstanceElementRevisions.size() &&
		         m_meshInstanceElementRevisions[index] > appliedRevision);

		ranges.push_back(
		    StructuredBufferElementRange{
		        .FirstElement = static_cast<std::uint32_t>(firstElement),
		        .ElementCount = static_cast<std::uint32_t>(index - firstElement)});
	}
}

void RenderGpuGeometryState::Reset() noexcept
{
	m_payloads = {};
	m_meshInstanceElementRevisions.clear();
	m_meshInstanceSlotScratch.clear();
	m_meshInstanceRevision = 0u;
	m_meshInstanceSlotRevision = 0u;
}

void RenderGpuGeometryState::UpdateMeshInstances(
    const RenderSceneData& sceneData)
{
	std::size_t requiredCapacity = 1u;
	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		requiredCapacity =
		    (std::max)(
		        requiredCapacity,
		        static_cast<std::size_t>(
		            draw.Source.GpuSceneSlot) +
		            1u);
	}

	const std::uint64_t nextRevision =
	    m_meshInstanceRevision + 1u;
	bool changed = false;
	if (requiredCapacity > m_payloads.MeshInstances.size())
	{
		const std::size_t previousCapacity =
		    m_payloads.MeshInstances.size();
		m_payloads.MeshInstances.resize(requiredCapacity);
		m_meshInstanceElementRevisions.resize(requiredCapacity);
		std::fill(
		    m_meshInstanceElementRevisions.begin() +
		        previousCapacity,
		    m_meshInstanceElementRevisions.end(),
		    nextRevision);
		changed = true;
	}

	for (const MeshDraw& draw : sceneData.meshInstances)
	{
		const std::size_t slot = draw.Source.GpuSceneSlot;
		const MeshInstanceData value = BuildMeshInstance(draw);
		if (HasSameMeshInstance(
		        m_payloads.MeshInstances[slot],
		        value))
		{
			continue;
		}

		m_payloads.MeshInstances[slot] = value;
		m_meshInstanceElementRevisions[slot] = nextRevision;
		changed = true;
	}

	if (changed)
	{
		m_meshInstanceRevision = nextRevision;
	}
}

void RenderGpuGeometryState::UpdateMeshInstanceSlots(
    const RenderSceneData& sceneData)
{
	m_meshInstanceSlotScratch.clear();
	m_meshInstanceSlotScratch.reserve(
	    (std::max<std::size_t>)(
	        sceneData.rasterMeshInstanceIndices.size(),
	        1u));
	for (const std::uint32_t drawIndex :
	     sceneData.rasterMeshInstanceIndices)
	{
		if (drawIndex >= sceneData.meshInstances.size())
		{
			continue;
		}

		m_meshInstanceSlotScratch.push_back(
		    sceneData.meshInstances[drawIndex]
		        .Source.GpuSceneSlot);
	}
	if (m_meshInstanceSlotScratch.empty())
	{
		m_meshInstanceSlotScratch.push_back(0u);
	}

	if (m_meshInstanceSlotScratch ==
	    m_payloads.MeshInstanceSlots)
	{
		return;
	}

	m_payloads.MeshInstanceSlots.swap(
	    m_meshInstanceSlotScratch);
	++m_meshInstanceSlotRevision;
}

void RenderGpuGeometryState::UpdateDeformation(
    const RenderSceneData& sceneData)
{
	m_payloads.JointMatrices.clear();
	m_payloads.PreviousJointMatrices.clear();
	m_payloads.JointMatrices.reserve(
	    (std::max<std::size_t>)(
	        sceneData.jointMatrices.size(),
	        1u));
	m_payloads.PreviousJointMatrices.reserve(
	    (std::max<std::size_t>)(
	        sceneData.previousJointMatrices.size(),
	        1u));

	if (sceneData.jointMatrices.empty())
	{
		const JointMatrixData identity{
		    .SkinningMTX =
		        MathUtils::IdentityFloat4x4()};
		m_payloads.JointMatrices.push_back(identity);
		m_payloads.PreviousJointMatrices.push_back(identity);
	}
	else
	{
		for (const DirectX::XMFLOAT4X4& matrix :
		     sceneData.jointMatrices)
		{
			m_payloads.JointMatrices.push_back(
			    JointMatrixData{
			        .SkinningMTX = matrix});
		}

		const std::vector<DirectX::XMFLOAT4X4>& previousMatrices =
		    sceneData.previousJointMatrices.size() ==
		            sceneData.jointMatrices.size()
		        ? sceneData.previousJointMatrices
		        : sceneData.jointMatrices;
		for (const DirectX::XMFLOAT4X4& matrix :
		     previousMatrices)
		{
			m_payloads.PreviousJointMatrices.push_back(
			    JointMatrixData{
			        .SkinningMTX = matrix});
		}
	}

	m_payloads.MorphWeights.assign(
	    sceneData.morphWeights.begin(),
	    sceneData.morphWeights.end());
	m_payloads.PreviousMorphWeights =
	    sceneData.previousMorphWeights.size() ==
	            sceneData.morphWeights.size()
	        ? sceneData.previousMorphWeights
	        : sceneData.morphWeights;
	if (m_payloads.MorphWeights.empty())
	{
		m_payloads.MorphWeights.push_back(0.0f);
		m_payloads.PreviousMorphWeights.push_back(0.0f);
	}
}

MeshInstanceData RenderGpuGeometryState::BuildMeshInstance(
    const MeshDraw& draw) noexcept
{
	return MeshInstanceData{
	    .WorldMTX = draw.Transform.WorldMatrix,
	    .PreviousWorldMTX =
	        draw.Transform.PreviousWorldMatrix,
	    .WorldInvTransposeMTX =
	        draw.Transform.WorldInvTranspose,
	    .MaterialSlot = draw.Material.Slot,
	    .Flags =
	        (draw.Geometry.MeshKind ==
	                     RenderMeshKind::Skeletal &&
	                 draw.Skinning.JointMatrixOffset !=
	                     kInvalidMeshInstanceJointMatrixOffset
	             ? MeshInstanceFlag_Skinned
	             : 0u) |
	        (draw.Morph.TargetCount > 0u &&
	                 draw.Morph.WeightOffset !=
	                     kInvalidMeshInstanceMorphWeightOffset
	             ? MeshInstanceFlag_Morphed
	             : 0u),
	    .JointMatrixOffset =
	        draw.Skinning.JointMatrixOffset,
	    .MorphWeightOffset =
	        draw.Morph.WeightOffset,
	    .MorphTargetCount =
	        draw.Morph.TargetCount,
	    .MorphTargetVertexCount =
	        draw.Morph.VertexCount,
	    .DebugData = draw.Source.GpuSceneSlot};
}

bool RenderGpuGeometryState::HasSameMeshInstance(
    const MeshInstanceData& left,
    const MeshInstanceData& right) noexcept
{
	return std::memcmp(
	           &left.WorldMTX,
	           &right.WorldMTX,
	           sizeof(left.WorldMTX)) == 0 &&
	       std::memcmp(
	           &left.PreviousWorldMTX,
	           &right.PreviousWorldMTX,
	           sizeof(left.PreviousWorldMTX)) == 0 &&
	       std::memcmp(
	           &left.WorldInvTransposeMTX,
	           &right.WorldInvTransposeMTX,
	           sizeof(left.WorldInvTransposeMTX)) == 0 &&
	       left.MaterialSlot == right.MaterialSlot &&
	       left.Flags == right.Flags &&
	       left.JointMatrixOffset == right.JointMatrixOffset &&
	       left.MorphWeightOffset == right.MorphWeightOffset &&
	       left.MorphTargetCount == right.MorphTargetCount &&
	       left.MorphTargetVertexCount ==
	           right.MorphTargetVertexCount &&
	       left.DebugData == right.DebugData;
}
