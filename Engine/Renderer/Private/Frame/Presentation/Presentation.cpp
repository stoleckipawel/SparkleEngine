#include "../../PCH.h"
#include "Frame/Presentation/Presentation.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Core/PassUtilities.h"
#include "Passes/Presentation/OutputEncodingPass.h"
#include "Passes/Presentation/ToneMappingPass.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

namespace
{
	PixelFormat ResolveEncodedIntermediateFormat(PixelFormat backBufferFormat) noexcept
	{
		switch (backBufferFormat)
		{
			case PixelFormat::R8G8B8A8_UNorm_Srgb:
				return PixelFormat::R8G8B8A8_UNorm;
			case PixelFormat::B8G8R8A8_UNorm_Srgb:
				return PixelFormat::B8G8R8A8_UNorm;
			default:
				return backBufferFormat;
		}
	}
}

void AddPresentationPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    PixelFormat backBufferFormat,
    const SceneRenderTargets& sceneTargets,
    FrameGraphTextureHandle exposure)
{
	const FrameGraphTextureHandle toneMappedColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "ToneMappedSceneColor",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        FrameRenderFormats::SceneColor));
	const FrameGraphTextureHandle encodedColor = builder.CreateTexture(
	    FrameGraphTextureDesc::CreateColor(
	        "EncodedSceneColor",
	        sceneExtent.Width,
	        sceneExtent.Height,
	        ResolveEncodedIntermediateFormat(backBufferFormat)));

	auto& toneMappingParameters = builder.AllocPassParameters<ToneMappingPass>();
	ToneMappingPass::DeclareResources(
	    builder,
	    sceneTargets.FinalSceneColor,
	    exposure,
	    toneMappedColor,
	    toneMappingParameters);
	builder.AddSizedComputeShaderPass<ToneMappingPass>(toneMappingParameters, sceneExtent.Width, sceneExtent.Height);

	auto& outputEncodingParameters = builder.AllocPassParameters<OutputEncodingPass>();
	OutputEncodingPass::DeclareResources(builder, toneMappedColor, encodedColor, outputEncodingParameters);
	builder.AddSizedComputeShaderPass<OutputEncodingPass>(outputEncodingParameters, sceneExtent.Width, sceneExtent.Height);
	PassUtilities::AddCopyTexturePass(builder, "CopyEncodedColorToBackBuffer", sceneTargets.BackBuffer, encodedColor);
}
