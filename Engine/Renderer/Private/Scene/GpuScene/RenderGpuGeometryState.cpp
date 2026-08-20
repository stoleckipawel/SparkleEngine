#include "PCH.h"
#include "Scene/GpuScene/RenderGpuGeometryState.h"

#include "Core/Public/Math/MathUtils.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <algorithm>
#include <cstring>

void RenderGpuGeometryState::Update(const PreparedRenderScene& preparedScene, const RenderView& view)
{
	UpdateMeshInstances(preparedScene);
	UpdateMeshInstanceSlots(preparedScene, view);
	UpdateDeformation(preparedScene);
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
		} while (index < m_meshInstanceElementRevisions.size() && m_meshInstanceElementRevisions[index] > appliedRevision);

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

void RenderGpuGeometryState::UpdateMeshInstances(const PreparedRenderScene& preparedScene)
{
	std::size_t requiredCapacity = 1u;
	for (const PreparedRenderPrimitive& primitive : preparedScene.primitives)
	{
		const MeshDraw& draw = primitive.Draw;
		requiredCapacity = (std::max) (requiredCapacity, static_cast<std::size_t>(draw.Source.GpuSceneSlot) + 1u);
	}

	const std::uint64_t nextRevision = m_meshInstanceRevision + 1u;
	bool changed = false;
	if (requiredCapacity > m_payloads.MeshInstances.size())
	{
		const std::size_t previousCapacity = m_payloads.MeshInstances.size();
		m_payloads.MeshInstances.resize(requiredCapacity);
		m_meshInstanceElementRevisions.resize(requiredCapacity);
		std::fill(m_meshInstanceElementRevisions.begin() + previousCapacity, m_meshInstanceElementRevisions.end(), nextRevision);
		changed = true;
	}

	for (const PreparedRenderPrimitive& primitive : preparedScene.primitives)
	{
		const MeshDraw& draw = primitive.Draw;
		const std::size_t slot = draw.Source.GpuSceneSlot;
		const MeshInstanceData value = BuildMeshInstance(draw);
		if (HasSameMeshInstance(m_payloads.MeshInstances[slot], value))
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

void RenderGpuGeometryState::UpdateMeshInstanceSlots(const PreparedRenderScene& preparedScene, const RenderView& view)
{
	m_meshInstanceSlotScratch.clear();
	m_meshInstanceSlotScratch.reserve((std::max<std::size_t>) (view.rasterPrimitiveIndices.size(), 1u));
	for (const std::uint32_t drawIndex : view.rasterPrimitiveIndices)
	{
		if (drawIndex >= preparedScene.primitives.size())
		{
			continue;
		}

		m_meshInstanceSlotScratch.push_back(preparedScene.primitives[drawIndex].Draw.Source.GpuSceneSlot);
	}
	if (m_meshInstanceSlotScratch.empty())
	{
		m_meshInstanceSlotScratch.push_back(0u);
	}

	if (m_meshInstanceSlotScratch == m_payloads.MeshInstanceSlots)
	{
		return;
	}

	m_payloads.MeshInstanceSlots.swap(m_meshInstanceSlotScratch);
	++m_meshInstanceSlotRevision;
}

void RenderGpuGeometryState::UpdateDeformation(const PreparedRenderScene& preparedScene)
{
	m_payloads.JointMatrices.clear();
	m_payloads.PreviousJointMatrices.clear();
	m_payloads.JointMatrices.reserve((std::max<std::size_t>) (preparedScene.jointMatrices.size(), 1u));
	m_payloads.PreviousJointMatrices.reserve((std::max<std::size_t>) (preparedScene.previousJointMatrices.size(), 1u));

	if (preparedScene.jointMatrices.empty())
	{
		const JointMatrixData identity{.Matrix = MathUtils::IdentityFloat4x4()};
		m_payloads.JointMatrices.push_back(identity);
		m_payloads.PreviousJointMatrices.push_back(identity);
	}
	else
	{
		for (const DirectX::XMFLOAT4X4& matrix : preparedScene.jointMatrices)
		{
			m_payloads.JointMatrices.push_back(JointMatrixData{.Matrix = matrix});
		}

		const std::vector<DirectX::XMFLOAT4X4>& previousMatrices =
		    preparedScene.previousJointMatrices.size() == preparedScene.jointMatrices.size() ? preparedScene.previousJointMatrices
		                                                                                     : preparedScene.jointMatrices;
		for (const DirectX::XMFLOAT4X4& matrix : previousMatrices)
		{
			m_payloads.PreviousJointMatrices.push_back(JointMatrixData{.Matrix = matrix});
		}
	}

	m_payloads.MorphWeights.assign(preparedScene.morphWeights.begin(), preparedScene.morphWeights.end());
	m_payloads.PreviousMorphWeights = preparedScene.previousMorphWeights.size() == preparedScene.morphWeights.size()
	    ? preparedScene.previousMorphWeights
	    : preparedScene.morphWeights;
	if (m_payloads.MorphWeights.empty())
	{
		m_payloads.MorphWeights.push_back(0.0f);
		m_payloads.PreviousMorphWeights.push_back(0.0f);
	}
}

MeshInstanceData RenderGpuGeometryState::BuildMeshInstance(const MeshDraw& draw) noexcept
{
	return MeshInstanceData{
	    .WorldMatrix = draw.Transform.WorldMatrix,
	    .PreviousWorldMatrix = draw.Transform.PreviousWorldMatrix,
	    .WorldInverseTranspose = draw.Transform.WorldInvTranspose,
	    .MaterialSlot = draw.Material.Slot,
	    .Flags =
	        (draw.Geometry.MeshKind == RenderMeshKind::Skeletal && draw.Skinning.JointMatrixOffset != kInvalidMeshInstanceJointMatrixOffset
	                ? MeshInstanceFlag_Skinned
	                : 0u)
	        | (draw.Morph.TargetCount > 0u && draw.Morph.WeightOffset != kInvalidMeshInstanceMorphWeightOffset ? MeshInstanceFlag_Morphed
	                                                                                                           : 0u),
	    .JointMatrixOffset = draw.Skinning.JointMatrixOffset,
	    .MorphWeightOffset = draw.Morph.WeightOffset,
	    .MorphTargetCount = draw.Morph.TargetCount,
	    .MorphTargetVertexCount = draw.Morph.VertexCount,
	    .GpuSceneSlot = draw.Source.GpuSceneSlot};
}

bool RenderGpuGeometryState::HasSameMeshInstance(const MeshInstanceData& left, const MeshInstanceData& right) noexcept
{
	return std::memcmp(&left.WorldMatrix, &right.WorldMatrix, sizeof(left.WorldMatrix)) == 0
	    && std::memcmp(&left.PreviousWorldMatrix, &right.PreviousWorldMatrix, sizeof(left.PreviousWorldMatrix)) == 0
	    && std::memcmp(&left.WorldInverseTranspose, &right.WorldInverseTranspose, sizeof(left.WorldInverseTranspose)) == 0
	    && left.MaterialSlot == right.MaterialSlot && left.Flags == right.Flags && left.JointMatrixOffset == right.JointMatrixOffset
	    && left.MorphWeightOffset == right.MorphWeightOffset && left.MorphTargetCount == right.MorphTargetCount
	    && left.MorphTargetVertexCount == right.MorphTargetVertexCount && left.GpuSceneSlot == right.GpuSceneSlot;
}
