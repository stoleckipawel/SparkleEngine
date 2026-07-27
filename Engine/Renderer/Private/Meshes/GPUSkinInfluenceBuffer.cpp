#include "PCH.h"

#include "Meshes/GPUSkinInfluenceBuffer.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

static const auto g_gpuSkinInfluenceBufferLogger = Logging::GetOrCreateLogger("Renderer.GPUSkinInfluenceBuffer");

GPUSkinInfluenceBuffer::~GPUSkinInfluenceBuffer() noexcept
{
	Release();
}

bool GPUSkinInfluenceBuffer::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    std::span<const VertexSkinInfluenceData> skinInfluences)
{
	Release();
	m_renderHardwareInterface = &renderHardwareInterface;

	if (skinInfluences.empty())
	{
		Release();
		return false;
	}

	if (!m_renderHardwareInterface->GetResourceService().CreateStructuredBuffer(
	        skinInfluences.data(),
	        skinInfluences.size_bytes(),
	        static_cast<std::uint32_t>(sizeof(VertexSkinInfluenceData)),
	        L"GPUMesh_SkinInfluences",
	        m_buffer,
	        m_view))
	{
		SPDLOG_LOGGER_ERROR(g_gpuSkinInfluenceBufferLogger, "GPUSkinInfluenceBuffer: failed to create skin influence buffer");
		Release();
		return false;
	}

	m_shaderResourceView = m_renderHardwareInterface->GetDescriptorService().GetResourceViewGpuHandle(m_view);
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
