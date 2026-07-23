#include "PCH.h"

#include "Passes/Core/RasterPassUtilities.h"

namespace RasterPassUtilities
{
	RenderPassDefinition BuildFullscreenDefinition(
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
}
