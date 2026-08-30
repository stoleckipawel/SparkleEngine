#include "PCH.h"

#include "Meshes/GpuSkinInfluenceBuffer.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Resources/RhiUploadService.h"

static const auto g_gpuSkinInfluenceBufferLogger = Logging::GetOrCreateLogger("Renderer.GpuSkinInfluenceBuffer");

GpuSkinInfluenceBuffer::~GpuSkinInfluenceBuffer() noexcept
{
	Release();
}

void GpuSkinInfluenceBuffer::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    RenderCommandList& commandList,
    std::span<const VertexSkinInfluenceData> skinInfluences)
{
	Release();
	m_renderHardwareInterface = &renderHardwareInterface;

	if (skinInfluences.empty())
		Diagnostics::Fatal(g_gpuSkinInfluenceBufferLogger, __FILE__, __LINE__, "GPU mesh has no skin influence payload.");

	RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();
	m_buffer = resources.CreateBufferResource(
	    RhiBufferResourceDesc{
	        .SizeInBytes = skinInfluences.size_bytes(),
	        .StrideInBytes = sizeof(VertexSkinInfluenceData),
	        .Kind = RhiBufferKind::Structured},
	    ResourceState::CopyDest,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::DeviceLocal,
	    L"GpuMesh_SkinInfluences");
	if (!m_buffer
	    || !m_renderHardwareInterface->GetUploadService().UploadBuffer(
	        commandList,
	        m_buffer,
	        std::as_bytes(skinInfluences),
	        ResourceState::ShaderResource,
	        L"GpuMesh_SkinInfluencesUpload"))
	{
		Release();
		Diagnostics::Fatal(g_gpuSkinInfluenceBufferLogger, __FILE__, __LINE__, "GPU skin influence upload failed.");
	}

	m_view = m_renderHardwareInterface->GetDescriptorService().CreateResourceView(
	    RhiResourceViewDesc::BufferShaderResource(
	        resources.GetResourceHandle(m_buffer),
	        skinInfluences.size_bytes(),
	        sizeof(VertexSkinInfluenceData)));
	m_shaderResourceView = m_renderHardwareInterface->GetDescriptorService().GetResourceViewGpuHandle(m_view);
	if (!m_shaderResourceView)
	{
		Release();
		Diagnostics::Fatal(
		    g_gpuSkinInfluenceBufferLogger,
		    __FILE__,
		    __LINE__,
		    "GPU skin influence buffer has no shader-resource descriptor.");
	}
}

void GpuSkinInfluenceBuffer::Release() noexcept
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
