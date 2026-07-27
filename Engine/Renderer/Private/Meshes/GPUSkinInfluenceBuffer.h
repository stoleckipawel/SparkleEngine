#pragma once

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"
#include "ShaderData/RenderConstantBufferData.h"

#include <span>

class RenderHardwareInterface;

class GPUSkinInfluenceBuffer final
{
  public:
	GPUSkinInfluenceBuffer() noexcept = default;
	~GPUSkinInfluenceBuffer() noexcept;

	GPUSkinInfluenceBuffer(const GPUSkinInfluenceBuffer&) = delete;
	GPUSkinInfluenceBuffer& operator=(const GPUSkinInfluenceBuffer&) = delete;
	GPUSkinInfluenceBuffer(GPUSkinInfluenceBuffer&&) = delete;
	GPUSkinInfluenceBuffer& operator=(GPUSkinInfluenceBuffer&&) = delete;

	bool Upload(
	    RenderHardwareInterface& renderHardwareInterface,
	    std::span<const VertexSkinInfluenceData> skinInfluences);
	void Release() noexcept;

	RhiGpuDescriptorHandle GetShaderResourceView() const noexcept { return m_shaderResourceView; }
	bool IsValid() const noexcept { return m_buffer && m_view && m_shaderResourceView; }

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_buffer = {};
	RhiResourceViewHandle m_view = {};
	RhiGpuDescriptorHandle m_shaderResourceView = {};
};
