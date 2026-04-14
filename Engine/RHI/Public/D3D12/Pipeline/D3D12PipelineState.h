#pragma once

#include "../Shaders/ShaderCompileResult.h"
#include "D3D12RootSignature.h"
#include "Config/RenderConfig.h"

#include <span>
#include <array>
#include <cstdint>
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

struct DepthTestDesc
{
	bool DepthEnable = true;
	D3D12_DEPTH_WRITE_MASK DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	D3D12_COMPARISON_FUNC DepthFunc = D3D12_COMPARISON_FUNC_LESS;
};

struct StencilTestDesc
{
	bool StencilEnable = false;
	uint8_t StencilReadMask = 0xFF;
	uint8_t StencilWriteMask = 0xFF;
	D3D12_COMPARISON_FUNC FrontFaceStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	D3D12_STENCIL_OP FrontFaceStencilFailOp = D3D12_STENCIL_OP_KEEP;
	D3D12_STENCIL_OP FrontFaceStencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	D3D12_STENCIL_OP FrontFaceStencilPassOp = D3D12_STENCIL_OP_KEEP;
	D3D12_COMPARISON_FUNC BackFaceStencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	D3D12_STENCIL_OP BackFaceStencilFailOp = D3D12_STENCIL_OP_KEEP;
	D3D12_STENCIL_OP BackFaceStencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	D3D12_STENCIL_OP BackFaceStencilPassOp = D3D12_STENCIL_OP_KEEP;
};

struct GraphicsPipelineStateDesc
{
	std::span<const D3D12_INPUT_ELEMENT_DESC> VertexLayout;
	D3D12RootSignature* RootSignature = nullptr;
	ShaderBytecode VertexShader = {};
	ShaderBytecode PixelShader = {};
	bool HasPixelShader = true;
	bool RenderWireframe = false;
	D3D12_CULL_MODE CullMode = D3D12_CULL_MODE_BACK;
	DepthTestDesc DepthTest = {};
	StencilTestDesc StencilTest = {};
	std::array<DXGI_FORMAT, 8> RenderTargetFormats = {};
	std::uint32_t RenderTargetCount = 1;
	DXGI_FORMAT DepthStencilFormat = RenderConfig::DepthStencilFormat;
	const wchar_t* DebugName = L"RHI_GraphicsPipelineState";
};

struct ComputePipelineStateDesc
{
	D3D12RootSignature* RootSignature = nullptr;
	ShaderBytecode ComputeShader = {};
	const wchar_t* DebugName = L"RHI_ComputePipelineState";
};

class D3D12PipelineState
{
  public:
	D3D12PipelineState(D3D12Rhi& rhi, const GraphicsPipelineStateDesc& desc);
	D3D12PipelineState(D3D12Rhi& rhi, const ComputePipelineStateDesc& desc);

	D3D12PipelineState(
	    D3D12Rhi& rhi,
	    std::span<const D3D12_INPUT_ELEMENT_DESC> vertexLayout,
	    D3D12RootSignature& rootSignature,
	    ShaderBytecode vertexShader,
	    ShaderBytecode pixelShader);

	~D3D12PipelineState() noexcept;

	D3D12PipelineState(const D3D12PipelineState&) = delete;
	D3D12PipelineState& operator=(const D3D12PipelineState&) = delete;

	const ComPtr<ID3D12PipelineState>& Get() const noexcept { return m_pso; }

  private:
	void HandlePsoCreateFailure(HRESULT hr) const noexcept;

	void SetStreamOutput(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) noexcept;
	void SetRasterizerState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, bool bRenderWireframe, D3D12_CULL_MODE cullMode) noexcept;
	void SetRenderTargetBlendState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, D3D12_RENDER_TARGET_BLEND_DESC blendDesc) noexcept;
	void SetDepthTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, DepthTestDesc depthDesc) noexcept;
	void SetStencilTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, StencilTestDesc stencilDesc) noexcept;
	void Create(const GraphicsPipelineStateDesc& desc);
	void Create(const ComputePipelineStateDesc& desc);

  private:
	D3D12Rhi& m_rhi;
	ComPtr<ID3D12PipelineState> m_pso = nullptr;
};
