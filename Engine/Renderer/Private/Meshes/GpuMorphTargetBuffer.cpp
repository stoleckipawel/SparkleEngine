#include "PCH.h"

#include "Meshes/GpuMorphTargetBuffer.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <algorithm>

GpuMorphTargetBuffer::GpuMorphTargetBuffer() noexcept = default;

GpuMorphTargetBuffer::~GpuMorphTargetBuffer() noexcept
{
	Release();
}

bool GpuMorphTargetBuffer::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    std::vector<MorphTargetDeltaData> deltas,
    std::uint32_t targetCount)
{
	Release();
	m_renderHardwareInterface = &renderHardwareInterface;
	m_deltas = std::move(deltas);
	m_targetCount = targetCount;

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
