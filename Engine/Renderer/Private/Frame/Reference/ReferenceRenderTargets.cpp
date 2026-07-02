#include "../../PCH.h"
#include "Frame/Reference/ReferenceRenderTargets.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/ResourceUsage.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <array>

namespace
{
	constexpr const char* kReferenceTargetClearPassName = "ReferenceTargetClear";

	FrameGraphTextureHandle CreateReferenceTexture(
	    FrameGraphBuilder& builder,
	    const char* name,
	    RenderViewportExtent sceneExtent,
	    PixelFormat format)
	{
		FrameGraphTextureDesc desc =
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, format);
		desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
		return builder.CreateTexture(desc);
	}

	auto GetReferenceTargets(const ReferenceRenderTargets& targets) noexcept
	{
		return std::array{
		    targets.ReferenceDirect,
		    targets.ReferenceIndirectDiffuse,
		    targets.ReferenceIndirectSpecular,
		    targets.ReferenceSceneColor,
		    targets.ReferencePrimaryDeviceDepth,
		    targets.ReferencePrimaryNormal,
		    targets.ReferencePrimaryDiffuseAlbedo,
		    targets.ReferencePrimarySpecularAlbedo,
		    targets.ReferencePrimaryMaterialGuide,
		    targets.ReferencePrimaryPathSampleGuide};
	}
}

ReferenceRenderTargets CreateReferenceRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	ReferenceRenderTargets targets{};
	targets.ReferenceDirect = CreateReferenceTexture(builder, "ReferenceDirect", sceneExtent, FrameRenderFormats::SceneColor);
	targets.ReferenceIndirectDiffuse =
	    CreateReferenceTexture(builder, "ReferenceIndirectDiffuse", sceneExtent, FrameRenderFormats::SceneColor);
	targets.ReferenceIndirectSpecular =
	    CreateReferenceTexture(builder, "ReferenceIndirectSpecular", sceneExtent, FrameRenderFormats::SceneColor);
	targets.ReferenceSceneColor = CreateReferenceTexture(builder, "ReferenceSceneColor", sceneExtent, FrameRenderFormats::SceneColor);
	targets.ReferencePrimaryDeviceDepth =
	    CreateReferenceTexture(builder, "ReferencePrimaryDeviceDepth", sceneExtent, PixelFormat::R32_Float);
	targets.ReferencePrimaryNormal =
	    CreateReferenceTexture(builder, "ReferencePrimaryNormal", sceneExtent, PixelFormat::R16G16B16A16_Float);
	targets.ReferencePrimaryDiffuseAlbedo =
	    CreateReferenceTexture(builder, "ReferencePrimaryDiffuseAlbedo", sceneExtent, PixelFormat::R32G32B32A32_Float);
	targets.ReferencePrimarySpecularAlbedo =
	    CreateReferenceTexture(builder, "ReferencePrimarySpecularAlbedo", sceneExtent, PixelFormat::R32G32B32A32_Float);
	targets.ReferencePrimaryMaterialGuide =
	    CreateReferenceTexture(builder, "ReferencePrimaryMaterialGuide", sceneExtent, PixelFormat::R32G32B32A32_Float);
	targets.ReferencePrimaryPathSampleGuide =
	    CreateReferenceTexture(builder, "ReferencePrimaryPathSampleGuide", sceneExtent, PixelFormat::R32G32B32A32_Float);
	return targets;
}

void AddReferenceTargetClearPass(FrameGraphBuilder& builder, const ReferenceRenderTargets& targets)
{
	builder.AddPass(
	    kReferenceTargetClearPassName,
	    EFrameGraphPassFlags::Raster,
	    [targets](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Write(targets.ReferenceDirect, ResourceUsage::RenderTarget, "ReferenceDirect");
		    resourceBuilder.Write(targets.ReferenceIndirectDiffuse, ResourceUsage::RenderTarget, "ReferenceIndirectDiffuse");
		    resourceBuilder.Write(targets.ReferenceIndirectSpecular, ResourceUsage::RenderTarget, "ReferenceIndirectSpecular");
		    resourceBuilder.Write(targets.ReferenceSceneColor, ResourceUsage::RenderTarget, "ReferenceSceneColor");
		    resourceBuilder.Write(targets.ReferencePrimaryDeviceDepth, ResourceUsage::RenderTarget, "ReferencePrimaryDeviceDepth");
		    resourceBuilder.Write(targets.ReferencePrimaryNormal, ResourceUsage::RenderTarget, "ReferencePrimaryNormal");
		    resourceBuilder.Write(targets.ReferencePrimaryDiffuseAlbedo, ResourceUsage::RenderTarget, "ReferencePrimaryDiffuseAlbedo");
		    resourceBuilder.Write(targets.ReferencePrimarySpecularAlbedo, ResourceUsage::RenderTarget, "ReferencePrimarySpecularAlbedo");
		    resourceBuilder.Write(targets.ReferencePrimaryMaterialGuide, ResourceUsage::RenderTarget, "ReferencePrimaryMaterialGuide");
		    resourceBuilder.Write(
		        targets.ReferencePrimaryPathSampleGuide,
		        ResourceUsage::RenderTarget,
		        "ReferencePrimaryPathSampleGuide");
	    },
	    [targets](PassExecutionContext& context)
	    {
		    for (FrameGraphTextureHandle target : GetReferenceTargets(targets))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }
	    });
}
