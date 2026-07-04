#pragma once

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "RHI/Public/Pipeline/RhiPipelineStateDesc.h"

#include <cassert>
#include <cstdint>
#include <string_view>

namespace RasterPassUtilities
{
	template <typename TPass>
	const typename TPass::ParameterMetadata& BuildParameterMetadata()
	{
		static const typename TPass::ParameterMetadata metadata = []
		{
			const typename TPass::ParameterMetadata localMetadata =
			    ShaderParameterStructBuilder<typename TPass::Parameters>::BuildMetadata(TPass::PassName);
			const bool valid = ValidateShaderPassLayout(localMetadata.GetLayout(), ShaderPassKind::Raster, TPass::PassName);
			assert(valid);
			return localMetadata;
		}();

		return metadata;
	}

	inline RenderPassDefinition BuildFullscreenDefinition(
	    const char* passName,
	    std::string_view packageId,
	    const wchar_t* bindingLayoutName,
	    const wchar_t* pipelineStateName,
	    PixelFormat renderTargetFormat,
	    bool usePresentColorFormat)
	{
		return RenderPassDefinition{
		    .PassName = passName,
		    .ShaderPackage = ShaderPackageDefinition{
		        .PackageId = packageId.data(),
		        .ExpectedStages = ShaderStageMask::Vertex | ShaderStageMask::Pixel},
		    .PipelineKind = RenderPassDefinitionPipelineKind::Graphics,
		    .AllowInputAssemblerInputLayout = true,
		    .BindingLayoutDebugName = bindingLayoutName,
		    .PipelineStateDebugName = pipelineStateName,
		    .Graphics = RenderPassGraphicsPipelineDefinition{
		        .CullMode = ERhiCullMode::None,
		        .DepthTest = RhiDepthTestDesc{.DepthEnable = false, .DepthWriteEnable = false},
		        .RenderTargetFormats = {renderTargetFormat},
		        .RenderTargetCount = 1,
		        .UsePresentColorFormat = usePresentColorFormat,
		        .DepthStencilFormat = PixelFormat::Unknown}};
	}

	template <typename TPass>
	bool DrawFullscreen(
	    PassExecutionContext& context,
	    const RasterPassPipelineRuntime& runtime,
	    typename TPass::ParameterInstance& parameters)
	{
		const bool valid = parameters.Sync();
		assert(valid);

		context.Commands.SetViewport(context.Frame.mainView.viewport);
		context.Commands.SetScissorRect(context.Frame.mainView.scissorRect);
		context.Resources.BindRenderTarget(context.Commands, parameters->RenderTarget[0]);

		const bool bound = RasterShaderPass<typename TPass::Parameters>::Bind(
		    context.Resources,
		    context.Commands,
		    &context.RuntimeServices.HardwareInterface,
		    runtime.BindingLayout,
		    runtime.PipelineState,
		    parameters,
		    nullptr,
		    0,
		    nullptr,
		    TPass::PassName);
		assert(bound);

		PassUtilities::DrawFullscreenTriangle(context.Commands);
		return bound;
	}
}
