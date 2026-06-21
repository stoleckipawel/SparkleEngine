#pragma once

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

	static bool TryCreateRuntimeStorage(
	    RenderHardwareInterface& rhi,
	    CookedShaderPackageCache& shaderPackageCache,
	    const RenderPassDefinition& definition,
	    RenderPassShaderRuntimeStorage& storage,
	    std::string& outErrorMessage)
	{
		if (!ValidateDefinition(definition, outErrorMessage))
		{
			return false;
		}

		const RenderPassShaderRuntimeDesc runtimeDesc{
		    .PassName = definition.PassName,
		    .PackageDeclarationName = definition.PackageDeclarationName,
		    .Package = definition.ShaderPackage,
		    .PipelineKind = ConvertPipelineKind(definition.PipelineKind),
		    .AllowInputAssemblerInputLayout = definition.AllowInputAssemblerInputLayout,
		    .BindingLayoutDebugName = definition.BindingLayoutDebugName,
		    .PipelineStateDebugName = definition.PipelineStateDebugName};

		if (definition.PipelineKind == RenderPassDefinitionPipelineKind::Graphics)
		{
			return RenderPassShaderRuntime::TryCreateGraphicsRuntime(
			    rhi,
			    shaderPackageCache,
			    runtimeDesc,
			    storage,
			    [&definition, &rhi](GraphicsPipelineStateDesc& pipelineDesc)
			    {
				    ApplyGraphicsDefinition(rhi, definition.Graphics, pipelineDesc);
			    },
			    outErrorMessage);
		}

		return RenderPassShaderRuntime::TryCreateComputeRuntime(
		    rhi,
		    shaderPackageCache,
		    runtimeDesc,
		    storage,
		    [](ComputePipelineStateDesc&)
		    {
		    },
		    outErrorMessage);
	}

	template <typename TRuntime> static TRuntime MakeRuntime(const RenderPassShaderRuntimeStorage& storage) noexcept
	{
		if constexpr (std::is_same_v<TRuntime, RasterPassPipelineRuntime>)
		{
			return TRuntime{*storage.BindingLayout, *storage.PipelineState, storage.WireframePipelineState.get(), storage.TwoSidedPipelineState.get()};
		}
		else
		{
			return TRuntime{*storage.BindingLayout, *storage.PipelineState};
		}
	}

  private:
	static RenderPassShaderPipelineKind ConvertPipelineKind(RenderPassDefinitionPipelineKind kind) noexcept
	{
		switch (kind)
		{
			case RenderPassDefinitionPipelineKind::Graphics:
				return RenderPassShaderPipelineKind::Graphics;
			case RenderPassDefinitionPipelineKind::Compute:
				return RenderPassShaderPipelineKind::Compute;
		}

		return RenderPassShaderPipelineKind::Compute;
	}

	static bool ValidateDefinition(const RenderPassDefinition& definition, std::string& outErrorMessage)
	{
		if (definition.PassName == nullptr || definition.PassName[0] == '\0')
		{
			outErrorMessage = "Render pass definition is missing a pass name";
			return false;
		}

		if (definition.PackageDeclarationName == nullptr || definition.PackageDeclarationName[0] == '\0')
		{
			outErrorMessage = std::format("Render pass '{}' is missing a package declaration name", definition.PassName);
			return false;
		}

		if (!definition.ShaderPackage.IsValid())
		{
			outErrorMessage = std::format("Render pass '{}' declares an invalid shader package", definition.PassName);
			return false;
		}

		if (definition.PipelineKind == RenderPassDefinitionPipelineKind::Graphics && definition.Graphics.RenderTargetCount == 0)
		{
			outErrorMessage = std::format("Render pass '{}' graphics definition declares no render targets", definition.PassName);
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	static void ApplyGraphicsDefinition(
	    RenderHardwareInterface& rhi,
	    const RenderPassGraphicsPipelineDefinition& definition,
	    GraphicsPipelineStateDesc& pipelineDesc) noexcept
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
			pipelineDesc.RenderTargetFormats[0] = rhi.GetPresentationService().GetPresentColorFormat();
		}
		pipelineDesc.DepthStencilFormat = definition.DepthStencilFormat;
	}
};
