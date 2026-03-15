#include "PCH.h"
#include "D3D12PipelineState.h"
#include "D3D12Rhi.h"
#include "DepthConvention.h"

#include <cstdio>
#include <vector>
#include <string>

void D3D12PipelineState::SetStreamOutput(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) noexcept
{
	psoDesc.StreamOutput = {};
}

void D3D12PipelineState::SetRasterizerState(
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
    bool bRenderWireframe,
    D3D12_CULL_MODE cullMode) noexcept
{
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	auto& rs = psoDesc.RasterizerState;
	rs = {};
	rs.FillMode = bRenderWireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
	rs.CullMode = cullMode;
	rs.FrontCounterClockwise = FALSE;
	rs.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rs.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rs.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rs.DepthClipEnable = TRUE;
	rs.MultisampleEnable = FALSE;
	rs.AntialiasedLineEnable = FALSE;
	rs.ForcedSampleCount = 0;
	rs.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

void D3D12PipelineState::SetRenderTargetBlendState(
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
    D3D12_RENDER_TARGET_BLEND_DESC blendDesc) noexcept
{
	psoDesc.BlendState = {};
	psoDesc.BlendState.RenderTarget[0] = blendDesc;
}

void D3D12PipelineState::SetDepthTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, DepthTestDesc depthDesc) noexcept
{
	auto& ds = psoDesc.DepthStencilState;
	ds = {};
	ds.DepthEnable = depthDesc.DepthEnable ? TRUE : FALSE;
	ds.DepthWriteMask = depthDesc.DepthWriteMask;
	ds.DepthFunc = depthDesc.DepthFunc;
}

void D3D12PipelineState::SetStencilTestState(D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc, StencilTestDesc stencilDesc) noexcept
{
	auto& ds = psoDesc.DepthStencilState;
	ds.StencilEnable = stencilDesc.StencilEnable ? TRUE : FALSE;
	ds.StencilReadMask = stencilDesc.StencilReadMask;
	ds.StencilWriteMask = stencilDesc.StencilWriteMask;

	ds.FrontFace.StencilFunc = stencilDesc.FrontFaceStencilFunc;
	ds.FrontFace.StencilFailOp = stencilDesc.FrontFaceStencilFailOp;
	ds.FrontFace.StencilDepthFailOp = stencilDesc.FrontFaceStencilDepthFailOp;
	ds.FrontFace.StencilPassOp = stencilDesc.FrontFaceStencilPassOp;

	ds.BackFace.StencilFunc = stencilDesc.BackFaceStencilFunc;
	ds.BackFace.StencilFailOp = stencilDesc.BackFaceStencilFailOp;
	ds.BackFace.StencilDepthFailOp = stencilDesc.BackFaceStencilDepthFailOp;
	ds.BackFace.StencilPassOp = stencilDesc.BackFaceStencilPassOp;
}

D3D12PipelineState::D3D12PipelineState(
    D3D12Rhi& rhi,
    std::span<const D3D12_INPUT_ELEMENT_DESC> vertexLayout,
    D3D12RootSignature& rootSignature,
    ShaderBytecode vertexShader,
    ShaderBytecode pixelShader) :
    m_rhi(rhi)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.InputLayout.NumElements = static_cast<UINT>(vertexLayout.size());
	psoDesc.InputLayout.pInputElementDescs = vertexLayout.data();
	psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

	psoDesc.pRootSignature = rootSignature.GetRaw();

	psoDesc.VS.pShaderBytecode = vertexShader.Data;
	psoDesc.VS.BytecodeLength = vertexShader.Size;
	psoDesc.PS.pShaderBytecode = pixelShader.Data;
	psoDesc.PS.BytecodeLength = pixelShader.Size;

	SetRasterizerState(psoDesc, false, D3D12_CULL_MODE_BACK);

	SetStreamOutput(psoDesc);

	psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
	psoDesc.BlendState.IndependentBlendEnable = FALSE;
	D3D12_RENDER_TARGET_BLEND_DESC rtBlend = {};
	rtBlend.BlendEnable = FALSE;
	rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlend.SrcBlend = D3D12_BLEND_ONE;
	rtBlend.DestBlend = D3D12_BLEND_ZERO;
	rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
	rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
	rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtBlend.LogicOpEnable = FALSE;
	rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	SetRenderTargetBlendState(psoDesc, rtBlend);

	DepthTestDesc depthTestDesc = {};
	depthTestDesc.DepthEnable = true;
	depthTestDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthTestDesc.DepthFunc = DepthConvention::GetDepthComparisonFuncEqual();
	SetDepthTestState(psoDesc, depthTestDesc);

	StencilTestDesc stencilDesc = {};
	SetStencilTestState(psoDesc, stencilDesc);

	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = RHISettings::BackBufferFormat;
	psoDesc.DSVFormat = RHISettings::DepthStencilFormat;

	psoDesc.NodeMask = 0;
	psoDesc.CachedPSO = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;

	HRESULT hr = m_rhi.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		HandlePsoCreateFailure(hr);
	}

	m_pso->SetName(L"RHI_PipelineState");
}

void D3D12PipelineState::HandlePsoCreateFailure(HRESULT hr) const noexcept
{
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(m_rhi.GetDevice()->QueryInterface(IID_PPV_ARGS(infoQueue.ReleaseAndGetAddressOf()))))
	{
		const UINT64 numMessages = infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
		for (UINT64 i = 0; i < numMessages; ++i)
		{
			SIZE_T messageLength = 0;
			if (FAILED(infoQueue->GetMessage(i, nullptr, &messageLength)) || messageLength == 0)
				continue;

			std::vector<char> messageData(messageLength);
			D3D12_MESSAGE* message = reinterpret_cast<D3D12_MESSAGE*>(messageData.data());
			if (SUCCEEDED(infoQueue->GetMessage(i, message, &messageLength)) && message->pDescription)
			{
				LOG_ERROR(std::string("D3D12 InfoQueue: ") + message->pDescription);
			}
		}

		infoQueue->ClearStoredMessages();
	}
#endif

	char buf[256];
	std::snprintf(buf, sizeof(buf), "Failed To Create PSO. HRESULT: 0x%08X", static_cast<unsigned int>(hr));
	LOG_FATAL(buf);
}

D3D12PipelineState::~D3D12PipelineState() noexcept
{
	m_pso.Reset();
}