#include "PCH.h"

#include "Meshes/GPUSkinInfluenceBuffer.h"

#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RhiUploadService.h"

static const auto g_gpuSkinInfluenceBufferLogger = Logging::GetOrCreateLogger("Renderer.GPUSkinInfluenceBuffer");

GPUSkinInfluenceBuffer::~GPUSkinInfluenceBuffer() noexcept
{
	Release();
}

bool GPUSkinInfluenceBuffer::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    RenderCommandList& commandList,
    std::span<const VertexSkinInfluenceData> skinInfluences)
{
	Release();
	m_renderHardwareInterface = &renderHardwareInterface;

	if (skinInfluences.empty())
	{
		Release();
		return false;
	}

	RhiResourceService& resources =
	    m_renderHardwareInterface->GetResourceService();
	m_buffer = resources.CreateBufferResource(
	    RhiBufferResourceDesc{
	        .SizeInBytes =
	            skinInfluences.size_bytes(),
	        .StrideInBytes =
	            sizeof(VertexSkinInfluenceData),
	        .Kind = RhiBufferKind::Structured},
	    ResourceState::CopyDest,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"GPUMesh_SkinInfluences");
	if (!m_buffer ||
	    !m_renderHardwareInterface->GetUploadService()
	         .UploadBuffer(
	             commandList,
	             m_buffer,
	             std::as_bytes(skinInfluences),
	             ResourceState::ShaderResource,
	             L"GPUMesh_SkinInfluencesUpload"))
	{
		SPDLOG_LOGGER_ERROR(
		    g_gpuSkinInfluenceBufferLogger,
		    "GPUSkinInfluenceBuffer: failed to create skin influence buffer");
		Release();
		return false;
	}

	m_view =
	    m_renderHardwareInterface->GetDescriptorService()
	        .CreateResourceView(
	            RhiResourceViewDesc::BufferShaderResource(
	                resources.GetResourceHandle(
	                    m_buffer),
	                skinInfluences.size_bytes(),
	                sizeof(
	                    VertexSkinInfluenceData)));
	m_shaderResourceView =
	    m_renderHardwareInterface->GetDescriptorService()
	        .GetResourceViewGpuHandle(m_view);
	if (!m_shaderResourceView)
	{
		SPDLOG_LOGGER_ERROR(g_gpuSkinInfluenceBufferLogger, "GPUSkinInfluenceBuffer: uploaded buffer has no shader-resource descriptor");
		Release();
		return false;
	}

	return true;
}

void GPUSkinInfluenceBuffer::Release() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	if (m_view)
	{
		m_renderHardwareInterface->GetDescriptorService().ReleaseResourceView(m_view);
		m_view = {};
	}

	if (m_buffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_buffer);
		m_buffer = {};
	}

	m_shaderResourceView = {};
	m_renderHardwareInterface = nullptr;
}
