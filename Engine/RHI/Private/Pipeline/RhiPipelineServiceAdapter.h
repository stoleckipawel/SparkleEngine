#pragma once

#include "Pipeline/RhiPipelineService.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <memory>

template <typename BackendRhi, typename BackendPipeline, typename BindingLayoutCompiler>
class RhiPipelineServiceAdapter final : public RhiPipelineService
{
  public:
	explicit RhiPipelineServiceAdapter(BackendRhi& rhi) noexcept : m_rhi(rhi) {}

	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) override
	{
		if (desc.ParameterLayout == nullptr || desc.Shaders.empty())
		{
			Fail("Binding-layout creation received an incomplete compile description.");
		}
		return BindingLayoutCompiler::Compile(m_rhi, desc);
	}

	std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) override
	{
		if (desc.BindingLayout == nullptr || !desc.VertexShader.IsValid())
		{
			Fail("Graphics-pipeline creation received an incomplete pipeline description.");
		}
		return std::make_unique<BackendPipeline>(m_rhi, desc);
	}

	std::unique_ptr<RenderPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override
	{
		if (desc.BindingLayout == nullptr || !desc.ComputeShader.IsValid())
		{
			Fail("Compute-pipeline creation received an incomplete pipeline description.");
		}
		return std::make_unique<BackendPipeline>(m_rhi, desc);
	}

  private:
	[[noreturn]] static void Fail(const char* message)
	{
		static const auto logger = Logging::GetOrCreateLogger("RHI.Pipeline");
		Diagnostics::Fatal(logger, __FILE__, __LINE__, message);
	}

	BackendRhi& m_rhi;
};
