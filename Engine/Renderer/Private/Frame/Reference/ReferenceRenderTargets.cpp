#include "../../PCH.h"
#include "Frame/Reference/ReferenceRenderTargets.h"

#include "Frame/Core/FrameRenderFormats.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/ResourceUsage.h"
#include "Renderer/Public/FrameGraph/FrameGraphTextureDesc.h"

#include <array>

namespace
{
	constexpr const char* kReferenceTargetClearPassName = "ReferenceTargetClear";

	FrameGraphTextureHandle CreateReferenceTexture(
	    FrameGraphBuilder& builder,
	    const char* name,
	    RenderViewportExtent sceneExtent)
	{
		FrameGraphTextureDesc desc =
		    FrameGraphTextureDesc::CreateColor(name, sceneExtent.Width, sceneExtent.Height, FrameRenderFormats::SceneColor);
		desc.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
		return builder.CreateTexture(desc);
	}

	auto GetReferenceTargets(const ReferenceRenderTargets& targets) noexcept
	{
		return std::array{
		    targets.ReferenceDirect,
		    targets.ReferenceIndirectDiffuse,
		    targets.ReferenceIndirectSpecular,
		    targets.ReferenceSceneColor};
	}
}

ReferenceRenderTargets CreateReferenceRenderTargets(FrameGraphBuilder& builder, RenderViewportExtent sceneExtent)
{
	ReferenceRenderTargets targets{};
	targets.ReferenceDirect = CreateReferenceTexture(builder, "ReferenceDirect", sceneExtent);
	targets.ReferenceIndirectDiffuse = CreateReferenceTexture(builder, "ReferenceIndirectDiffuse", sceneExtent);
	targets.ReferenceIndirectSpecular = CreateReferenceTexture(builder, "ReferenceIndirectSpecular", sceneExtent);
	targets.ReferenceSceneColor = CreateReferenceTexture(builder, "ReferenceSceneColor", sceneExtent);
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
	    },
	    [targets](PassExecutionContext& context)
	    {
		    for (FrameGraphTextureHandle target : GetReferenceTargets(targets))
		    {
			    context.Resources.ClearRenderTarget(context.Commands, target);
		    }
	    });
}
