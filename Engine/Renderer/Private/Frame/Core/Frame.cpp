#include "../../PCH.h"
#include "Frame/Core/Frame.h"

#include "Frame/Core/FrameSceneResources.h"
#include "Frame/Deferred/GBuffer.h"
#include "Frame/Lighting/Lighting.h"
#include "Frame/PostProcessing/PostProcessing.h"
#include "Frame/RayTracing/RayTracingScene.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

FrameBuildResult BuildFrame(FrameGraphBuilder& builder, const FrameBuildSettings& settings)
{
	FrameAssemblyResourceLayout resources = {};
	CreateFrameSceneResources(builder, settings, resources);
	AddRaytracingScenePasses(builder, resources);
	AddGBufferPasses(builder, settings.RenderExtent, resources);
	AddLightingPasses(builder, settings.RenderExtent, resources);
	AddPreReconstructionPostProcessingPasses(builder, settings, resources);
	AddLightingReconstructionPasses(builder, settings.RenderExtent, settings.OutputExtent, resources);

	AddPostProcessingPasses(builder, settings, resources);

	return FrameBuildResult{.Resources = resources};
}
