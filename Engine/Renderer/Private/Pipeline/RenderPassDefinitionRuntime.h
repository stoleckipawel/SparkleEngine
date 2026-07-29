#pragma once

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Pipeline/RenderPassShaderRuntime.h"

#include <format>
#include <string>
#include <type_traits>

class RenderPassDefinitionRuntime final
{
  public:
	RenderPassDefinitionRuntime() = delete;

	static void CreateRuntimeStorage(
	    RenderHardwareInterface& renderHardwareInterface,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassDefinition& definition,
	    RenderPassShaderRuntimeStorage& storage)
	{
		ValidateDefinition(definition);

		const RenderPassShaderRuntimeDesc runtimeDesc{
		    .PassName = definition.PassName,
		    .Package = definition.ShaderPackage,
		    .PipelineKind = ConvertPipelineKind(definition.PipelineKind),
		    .AllowInputAssemblerInputLayout = definition.AllowInputAssemblerInputLayout,
		    .BindingLayoutDebugName = definition.BindingLayoutDebugName,
		    .PipelineDebugName = definition.PipelineDebugName};

		if (definition.PipelineKind == RenderPassDefinitionPipelineKind::Graphics)
		{
			RenderPassShaderRuntime::CreateGraphicsRuntime(
			    renderHardwareInterface,
			    shaderPackageCache,
			    runtimeDesc,
			    storage,
			    [&definition, &renderHardwareInterface](GraphicsPipelineDesc& pipelineDesc)
			    {
				    ApplyGraphicsDefinition(renderHardwareInterface, definition.Graphics, pipelineDesc);
			    });
			return;
		}

		RenderPassShaderRuntime::CreateComputeRuntime(
		    renderHardwareInterface,
		    shaderPackageCache,
		    runtimeDesc,
		    storage,
		    [](ComputePipelineDesc&)
		    {
		    });
	}

	template <typename TRuntime> static TRuntime MakeRuntime(const RenderPassShaderRuntimeStorage& storage) noexcept
	{
		if constexpr (std::is_same_v<TRuntime, RasterPassPipelineRuntime>)
		{
			return TRuntime{*storage.BindingLayout, *storage.Pipeline, storage.WireframePipeline.get(), storage.TwoSidedPipeline.get()};
		}
		else
		{
			return TRuntime{*storage.BindingLayout, *storage.Pipeline};
		}
	}

  private:
	static RenderPassShaderPipelineKind ConvertPipelineKind(RenderPassDefinitionPipelineKind kind)
	{
		switch (kind)
		{
			case RenderPassDefinitionPipelineKind::Graphics:
				return RenderPassShaderPipelineKind::Graphics;
			case RenderPassDefinitionPipelineKind::Compute:
				return RenderPassShaderPipelineKind::Compute;
		}

		throw Diagnostics::Error("Render pass definition declares an unknown pipeline kind.");
	}

	static void ValidateDefinition(const RenderPassDefinition& definition)
	{
		if (definition.PassName == nullptr || definition.PassName[0] == '\0')
		{
			throw Diagnostics::Error("Render pass definition is missing a pass name.");
		}

		if (!definition.ShaderPackage.IsValid())
		{
			throw Diagnostics::Error(std::format("Render pass '{}' declares an invalid shader package.", definition.PassName));
		}

		if (definition.PipelineKind == RenderPassDefinitionPipelineKind::Graphics && definition.Graphics.RenderTargetCount == 0)
		{
			throw Diagnostics::Error(
			    std::format("Render pass '{}' graphics definition declares no render targets.", definition.PassName));
		}
	}

	static void ApplyGraphicsDefinition(
	    RenderHardwareInterface& renderHardwareInterface,
	    const RenderPassGraphicsPipelineDefinition& definition,
	    GraphicsPipelineDesc& pipelineDesc) noexcept
	{
		pipelineDesc.VertexLayout = definition.VertexLayout;
		pipelineDesc.RenderWireframe = definition.RenderWireframe;
		pipelineDesc.CullMode = definition.CullMode;
		pipelineDesc.FrontFaceWinding = definition.FrontFaceWinding;
		pipelineDesc.DepthTest = definition.DepthTest;
		pipelineDesc.StencilTest = definition.StencilTest;
		pipelineDesc.RenderTargetFormats = definition.RenderTargetFormats;
		pipelineDesc.RenderTargetCount = definition.RenderTargetCount;
		if (definition.UsePresentColorFormat && definition.RenderTargetCount > 0)
		{
			pipelineDesc.RenderTargetFormats[0] = renderHardwareInterface.GetPresentationService().GetPresentColorFormat();
		}
		pipelineDesc.DepthStencilFormat = definition.DepthStencilFormat;
	}
};
