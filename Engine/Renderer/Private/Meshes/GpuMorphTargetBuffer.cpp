#include "PCH.h"

#include "Meshes/GpuMorphTargetBuffer.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Meshes/MeshMorphData.h"
#include "ShaderData/MorphTargetShaderData.h"

#include <algorithm>
#include <vector>

class GpuMorphTargetBufferOperations final
{
  public:
	static bool BuildCpuPayload(
	    std::uint32_t vertexCount,
	    const MeshMorphData* morphTargets,
	    std::vector<MorphTargetDeltaData>& outDeltas,
	    std::uint32_t& outTargetCount);
	static MorphTargetDeltaData Convert(
	    const MeshMorphTargetDelta& delta) noexcept;
};

bool GpuMorphTargetBufferOperations::BuildCpuPayload(
    std::uint32_t vertexCount,
    const MeshMorphData* morphTargets,
    std::vector<MorphTargetDeltaData>& outDeltas,
    std::uint32_t& outTargetCount)
{
	outDeltas.clear();
	outTargetCount = 0u;
	if (morphTargets == nullptr || !morphTargets->HasTargets())
	{
		return true;
	}

	outDeltas.reserve(
	    static_cast<std::size_t>(vertexCount) *
	    morphTargets->targets.size());
	for (const MeshMorphTarget& target : morphTargets->targets)
	{
		if (!target.IsValidForVertexCount(vertexCount))
		{
			outDeltas.clear();
			return false;
		}
		for (const MeshMorphTargetDelta& delta : target.deltas)
		{
			outDeltas.push_back(Convert(delta));
		}
	}
	outTargetCount =
	    static_cast<std::uint32_t>(morphTargets->targets.size());
	return true;
}

MorphTargetDeltaData GpuMorphTargetBufferOperations::Convert(
    const MeshMorphTargetDelta& delta) noexcept
{
	return MorphTargetDeltaData{
	    .Position =
	        {delta.position.x,
	         delta.position.y,
	         delta.position.z,
	         0.0f},
	    .Normal =
	        {delta.normal.x,
	         delta.normal.y,
	         delta.normal.z,
	         0.0f},
	    .Tangent =
	        {delta.tangent.x,
	         delta.tangent.y,
	         delta.tangent.z,
	         0.0f}};
}

GpuMorphTargetBuffer::GpuMorphTargetBuffer() noexcept = default;

GpuMorphTargetBuffer::~GpuMorphTargetBuffer() noexcept
{
	Release();
}

bool GpuMorphTargetBuffer::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    std::uint32_t vertexCount,
    const MeshMorphData* morphTargets)
{
	Release();
	m_renderHardwareInterface = &renderHardwareInterface;
	if (!GpuMorphTargetBufferOperations::BuildCpuPayload(
	        vertexCount,
	        morphTargets,
	        m_deltas,
	        m_targetCount))
	{
		Release();
		return false;
	}

	const MorphTargetDeltaData emptyDelta = {};
	const MorphTargetDeltaData* gpuPayload =
	    m_deltas.empty() ? &emptyDelta : m_deltas.data();
	const std::size_t gpuPayloadCount =
	    std::max<std::size_t>(m_deltas.size(), 1u);
	if (!m_renderHardwareInterface->GetResourceService()
	         .CreateStructuredBuffer(
	             gpuPayload,
	             gpuPayloadCount *
	                 sizeof(MorphTargetDeltaData),
	             static_cast<std::uint32_t>(
	                 sizeof(MorphTargetDeltaData)),
	             L"GPUMesh_MorphTargetDeltas",
	             m_buffer,
	             m_view))
	{
		Release();
		return false;
	}

	m_shaderResourceView =
	    m_renderHardwareInterface->GetDescriptorService()
	        .GetResourceViewGpuHandle(m_view);
	if (!m_shaderResourceView)
	{
		Release();
		return false;
	}
	return true;
}

void GpuMorphTargetBuffer::Release() noexcept
{
	if (m_renderHardwareInterface != nullptr)
	{
		if (m_view)
		{
			m_renderHardwareInterface->GetDescriptorService()
			    .ReleaseResourceView(m_view);
		}
		if (m_buffer)
		{
			m_renderHardwareInterface->GetResourceService()
			    .ReleaseOwnedResource(m_buffer);
		}
	}

	m_renderHardwareInterface = nullptr;
	m_buffer = {};
	m_view = {};
	m_shaderResourceView = {};
	m_deltas.clear();
	m_targetCount = 0u;
}
