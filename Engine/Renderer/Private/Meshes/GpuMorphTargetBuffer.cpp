#include "PCH.h"

#include "Meshes/GpuMorphTargetBuffer.h"

#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RhiUploadService.h"

#include <algorithm>

GpuMorphTargetBuffer::GpuMorphTargetBuffer() noexcept = default;

GpuMorphTargetBuffer::~GpuMorphTargetBuffer() noexcept
{
	Release();
}

bool GpuMorphTargetBuffer::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    RenderCommandList& commandList,
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
	const std::span<const MorphTargetDeltaData> payload{
	    gpuPayload,
	    gpuPayloadCount};
	RhiResourceService& resources =
	    m_renderHardwareInterface->GetResourceService();
	m_buffer = resources.CreateBufferResource(
	    RhiBufferResourceDesc{
	        .SizeInBytes = payload.size_bytes(),
	        .StrideInBytes =
	            sizeof(MorphTargetDeltaData),
	        .Kind = RhiBufferKind::Structured},
	    ResourceState::CopyDest,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"GPUMesh_MorphTargetDeltas");
	if (!m_buffer ||
	    !m_renderHardwareInterface->GetUploadService()
	         .UploadBuffer(
	             commandList,
	             m_buffer,
	             std::as_bytes(payload),
	             ResourceState::ShaderResource,
	             L"GPUMesh_MorphTargetUpload"))
	{
		Release();
		return false;
	}

	m_view =
	    m_renderHardwareInterface->GetDescriptorService()
	        .CreateResourceView(
	            RhiResourceViewDesc::BufferShaderResource(
	                resources.GetResourceHandle(
	                    m_buffer),
	                payload.size_bytes(),
	                sizeof(
	                    MorphTargetDeltaData)));
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
