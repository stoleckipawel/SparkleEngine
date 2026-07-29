#include "PCH.h"

#include "Passes/Core/RasterPassOperations.h"

namespace RasterPassOperations
{
	RenderPassDefinition BuildFullscreenDefinition(
	    const char* passName,
	    std::string_view packageId,
	    const wchar_t* bindingLayoutName,
	    const wchar_t* pipelineName,
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
		    .PipelineDebugName = pipelineName,
		    .Graphics = RenderPassGraphicsPipelineDefinition{
		        .CullMode = ERhiCullMode::None,
		        .DepthTest = RhiDepthTestDesc{.DepthEnable = false, .DepthWriteEnable = false},
		        .RenderTargetFormats = {renderTargetFormat},
		        .RenderTargetCount = 1,
		        .UsePresentColorFormat = usePresentColorFormat,
		        .DepthStencilFormat = PixelFormat::Unknown}};
	}
}
