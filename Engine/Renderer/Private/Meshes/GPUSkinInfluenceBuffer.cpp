#include "PCH.h"

#include "Meshes/GPUSkinInfluenceBuffer.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "Scene/Meshes/MeshSkinningData.h"

#include <vector>

static const auto g_gpuSkinInfluenceBufferLogger = Logging::GetOrCreateLogger("Renderer.GPUSkinInfluenceBuffer");

class GPUSkinInfluenceBufferOperations final
{
  public:
	static VertexSkinInfluenceData ToGpuSkinInfluence(const VertexSkinInfluence& influence) noexcept
	{
		return VertexSkinInfluenceData{
		    .JointIndices =
		        {influence.jointIndices[0],
		         influence.jointIndices[1],
		         influence.jointIndices[2],
		         influence.jointIndices[3]},
		    .JointWeights =
		        {influence.jointWeights[0],
		         influence.jointWeights[1],
		         influence.jointWeights[2],
		         influence.jointWeights[3]}};
	}
};

GPUSkinInfluenceBuffer::~GPUSkinInfluenceBuffer() noexcept
{
	Release();
}

bool GPUSkinInfluenceBuffer::Upload(
    RenderHardwareInterface& renderHardwareInterface,
    std::uint32_t vertexCount,
    std::span<const VertexSkinInfluence> skinInfluences)
{
	Release();
	m_renderHardwareInterface = &renderHardwareInterface;

	std::vector<VertexSkinInfluenceData> gpuSkinInfluences;
	gpuSkinInfluences.reserve(vertexCount);
	if (skinInfluences.size() == vertexCount)
	{
		for (const VertexSkinInfluence& influence : skinInfluences)
		{
			gpuSkinInfluences.push_back(GPUSkinInfluenceBufferOperations::ToGpuSkinInfluence(influence));
		}
	}
	else
	{
		gpuSkinInfluences.resize(vertexCount);
	}

	if (!m_renderHardwareInterface->GetResourceService().CreateStructuredBuffer(
	        gpuSkinInfluences.data(),
	        gpuSkinInfluences.size() * sizeof(VertexSkinInfluenceData),
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
