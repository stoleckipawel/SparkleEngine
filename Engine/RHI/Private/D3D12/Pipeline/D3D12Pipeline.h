#pragma once

#include "Device/RenderHardwareInterface.h"
#include "D3D12RootSignature.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

class D3D12Pipeline final : public RenderPipeline
{
  public:
	D3D12Pipeline(D3D12Rhi& rhi, const GraphicsPipelineDesc& desc);
	D3D12Pipeline(D3D12Rhi& rhi, const ComputePipelineDesc& desc);

	~D3D12Pipeline() noexcept;

	D3D12Pipeline(const D3D12Pipeline&) = delete;
	D3D12Pipeline& operator=(const D3D12Pipeline&) = delete;

	const ComPtr<ID3D12PipelineState>& Get() const noexcept { return m_pso; }

  private:
	void HandlePsoCreateFailure(HRESULT hr) const noexcept;

	void SetStreamOutput(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) noexcept;
	void SetRasterizerState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, const RhiRasterizerState& rasterizer) noexcept;
	void SetRenderTargetBlendState(
	    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
	    const RhiBlendState& blend,
	    std::uint32_t colorAttachmentCount) noexcept;
	void SetDepthTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiDepthState depthDesc) noexcept;
	void SetStencilTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiStencilState stencilDesc) noexcept;
	void Create(const GraphicsPipelineDesc& desc);
	void Create(const ComputePipelineDesc& desc);

  private:
	D3D12Rhi& m_rhi;
	ComPtr<ID3D12PipelineState> m_pso = nullptr;
};
