#pragma once

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"
#include "ShaderData/RenderConstantBufferData.h"

#include <span>

class RenderHardwareInterface;
class RenderCommandList;

class GpuSkinInfluenceBuffer final
{
  public:
	GpuSkinInfluenceBuffer() noexcept = default;
	~GpuSkinInfluenceBuffer() noexcept;

	GpuSkinInfluenceBuffer(const GpuSkinInfluenceBuffer&) = delete;
	GpuSkinInfluenceBuffer& operator=(const GpuSkinInfluenceBuffer&) = delete;
	GpuSkinInfluenceBuffer(GpuSkinInfluenceBuffer&&) = delete;
	GpuSkinInfluenceBuffer& operator=(GpuSkinInfluenceBuffer&&) = delete;

	void Upload(
	    RenderHardwareInterface& renderHardwareInterface,
	    RenderCommandList& commandList,
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
