#include "D3D12/Pipeline/D3D12PipelineService.h"

#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Pipeline/D3D12BindingLayout.h"
#include "D3D12/Pipeline/D3D12Pipeline.h"

D3D12PipelineService::D3D12PipelineService(D3D12Rhi& rhi) noexcept : m_rhi(&rhi) {}

std::unique_ptr<RenderBindingLayout> D3D12PipelineService::CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc)
{
	if (m_rhi == nullptr || desc.ParameterLayout == nullptr || desc.ShaderPackage == nullptr)
	{
		return {};
	}

	return D3D12BindingLayoutCompiler::Compile(*m_rhi, desc);
}

std::unique_ptr<RenderPipeline> D3D12PipelineService::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.VertexShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12Pipeline>(*m_rhi, desc);
}

std::unique_ptr<RenderPipeline> D3D12PipelineService::CreateComputePipeline(const ComputePipelineDesc& desc)
{
	if (m_rhi == nullptr || desc.BindingLayout == nullptr || !desc.ComputeShader.IsValid())
	{
		return {};
	}

	return std::make_unique<D3D12Pipeline>(*m_rhi, desc);
}
