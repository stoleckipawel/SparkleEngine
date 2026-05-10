#pragma once

#include "Config/RenderConfig.h"
#include "Interop/RenderHardwareInterface.h"
#include "D3D12RootSignature.h"

#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

class D3D12PipelineState final : public RenderPipelineState
{
  public:
	D3D12PipelineState(D3D12Rhi& rhi, const GraphicsPipelineStateDesc& desc);
	D3D12PipelineState(D3D12Rhi& rhi, const ComputePipelineStateDesc& desc);

	~D3D12PipelineState() noexcept;

	D3D12PipelineState(const D3D12PipelineState&) = delete;
	D3D12PipelineState& operator=(const D3D12PipelineState&) = delete;

	const ComPtr<ID3D12PipelineState>& Get() const noexcept { return m_pso; }

  private:
	void HandlePsoCreateFailure(HRESULT hr) const noexcept;

	void SetStreamOutput(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) noexcept;
	void SetRasterizerState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, bool bRenderWireframe, ERhiCullMode cullMode) noexcept;
	void SetRenderTargetBlendState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, D3D12_RENDER_TARGET_BLEND_DESC blendDesc) noexcept;
	void SetDepthTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiDepthTestDesc depthDesc) noexcept;
	void SetStencilTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, RhiStencilTestDesc stencilDesc) noexcept;
	void Create(const GraphicsPipelineStateDesc& desc);
	void Create(const ComputePipelineStateDesc& desc);

  private:
	D3D12Rhi& m_rhi;
	ComPtr<ID3D12PipelineState> m_pso = nullptr;
};
