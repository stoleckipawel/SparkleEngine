#pragma once

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

}
