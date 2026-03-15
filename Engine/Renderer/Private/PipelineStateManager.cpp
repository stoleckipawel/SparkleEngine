#include "PCH.h"
#include "PipelineStateManager.h"

#include "D3D12Rhi.h"
#include "DxcShaderCompiler.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12VertexLayout.h"
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

D3D12PipelineState& PipelineStateManager::GetPipelineState() const noexcept
{
	return *m_pipelineState;
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
}

void PipelineStateManager::CreatePipelineState()
{
	m_pipelineState = std::make_unique<D3D12PipelineState>(
	    *m_rhi,
	    D3D12VertexLayout::GetStaticMeshLayout(),
	    *m_rootSignature,
	    m_vertexShader->GetBytecode(),
	    m_pixelShader->GetBytecode());
}
