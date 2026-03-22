#include "PCH.h"
#include "PipelineStateManager.h"

#include "D3D12Rhi.h"
#include "DxcShaderCompiler.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12VertexLayout.h"
#include "Renderer/Public/DepthConvention.h"
#include "RenderConfig.h"
#include "ShaderCompileResult.h"

PipelineStateManager::PipelineStateManager(D3D12Rhi& rhi) noexcept : m_rhi(&rhi)
{
	CreateRootSignature();
	CompileShaders();
	CreatePipelineState();
}

PipelineStateManager::~PipelineStateManager() noexcept = default;

D3D12RootSignature& PipelineStateManager::GetRootSignature() const noexcept
{
	return *m_rootSignature;
}

D3D12PipelineState& PipelineStateManager::GetForwardPipelineState() const noexcept
{
	return *m_forwardPipelineState;
}

D3D12PipelineState& PipelineStateManager::GetShadowPipelineState() const noexcept
{
	return *m_shadowPipelineState;
}

void PipelineStateManager::CreateRootSignature()
{
	m_rootSignature = std::make_unique<D3D12RootSignature>(*m_rhi);
}

void PipelineStateManager::CompileShaders()
{
	m_vertexShader = std::make_unique<ShaderCompileResult>(
	    DxcShaderCompiler::CompileFromAsset("Passes/Forward/ForwardLitVS.hlsl", ShaderStage::Vertex, "main"));
	m_pixelShader = std::make_unique<ShaderCompileResult>(
	    DxcShaderCompiler::CompileFromAsset("Passes/Forward/ForwardLitPS.hlsl", ShaderStage::Pixel, "main"));
	m_shadowVertexShader = std::make_unique<ShaderCompileResult>(
	    DxcShaderCompiler::CompileFromAsset("Passes/Shadow/ShadowDepthVS.hlsl", ShaderStage::Vertex, "main"));
	m_shadowPixelShader = std::make_unique<ShaderCompileResult>(
	    DxcShaderCompiler::CompileFromAsset("Passes/Shadow/ShadowDepthPS.hlsl", ShaderStage::Pixel, "main"));
}

void PipelineStateManager::CreatePipelineState()
{
	m_forwardPipelineState = std::make_unique<D3D12PipelineState>(
	    *m_rhi,
	    D3D12VertexLayout::GetStaticMeshLayout(),
	    *m_rootSignature,
	    m_vertexShader->GetBytecode(),
	    m_pixelShader->GetBytecode());

	GraphicsPipelineStateDesc shadowDesc{};
	shadowDesc.VertexLayout = D3D12VertexLayout::GetStaticMeshLayout();
	shadowDesc.RootSignature = m_rootSignature.get();
	shadowDesc.VertexShader = m_shadowVertexShader->GetBytecode();
	shadowDesc.PixelShader = m_shadowPixelShader->GetBytecode();
	shadowDesc.HasPixelShader = true;
	shadowDesc.DepthTest.DepthEnable = true;
	shadowDesc.DepthTest.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowDesc.DepthTest.DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc();
	shadowDesc.RenderTargetFormats[0] = RenderConfig::Shadows::ShadowMapFormat;
	shadowDesc.RenderTargetCount = 1;
	shadowDesc.DepthStencilFormat = RenderConfig::DepthStencilFormat;
	m_shadowPipelineState = std::make_unique<D3D12PipelineState>(*m_rhi, shadowDesc);
}
