#pragma once

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/FrameContext.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "Passes/Core/ShaderPassOperations.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "Passes/Core/ShaderPass.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "RHI/Public/Pipeline/RhiPipelineDesc.h"

#include <cassert>
#include <cstdint>
#include <string_view>

namespace RasterPassOperations
{
	template <typename TPass> const typename TPass::ParameterMetadata& BuildParameterMetadata()
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

	RenderPassDefinition BuildFullscreenDefinition(
	    const char* passName,
	    std::string_view packageId,
	    const wchar_t* bindingLayoutName,
	    const wchar_t* pipelineName,
	    PixelFormat renderTargetFormat,
	    bool usePresentColorFormat);

	template <typename TPass> bool DrawFullscreen(
	    PassExecutionContext& context,
	    const RasterPassPipelineRuntime& runtime,
	    typename TPass::ParameterInstance& parameters)
	{
		const bool valid = parameters.Sync();
		assert(valid);

		context.Commands.SetViewport(context.Frame.view.viewport);
		context.Commands.SetScissorRect(context.Frame.view.scissorRect);
		context.Resources.BindRenderTarget(context.Commands, parameters->RenderTarget[0]);

		const bool bound = RasterShaderPass<typename TPass::Parameters>::Bind(
		    context.Resources,
		    context.Commands,
		    &context.Runtime.HardwareInterface,
		    runtime.BindingLayout,
		    runtime.Pipeline,
		    parameters,
		    nullptr,
		    0,
		    nullptr,
		    TPass::PassName);
		assert(bound);

		ShaderPassOperations::DrawFullscreenTriangle(context.Commands);
		return bound;
	}
}
