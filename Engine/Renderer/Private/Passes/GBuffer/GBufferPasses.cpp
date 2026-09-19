#include "PCH.h"
#include "Passes/GBuffer/GBufferPasses.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Debug/RendererCVars.h"
#include "Passes/GBuffer/GBufferRenderTargets.h"
#include "Passes/GBuffer/SceneDepth.h"
#include "Passes/GBuffer/RayTracingGBuffer.h"
#include "Passes/GBuffer/RasterizedGBuffer.h"
#include "Passes/GBuffer/SkyMotionVectors.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"

void AddGBufferPasses(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    RenderViewportExtent sceneExtent,
    RenderFrameGraphResources& resources)
{
	CreateGBufferRenderTargets(builder, sceneExtent, resources);

	switch (CVarGBufferAlgorithm.Get())
	{
		case GBufferAlgorithm::Rasterized:
			AddRasterizedGBufferMeshPass(builder, gpuMeshCache, resources);
			break;
		case GBufferAlgorithm::RayTracing:
		{
			AddRayTracingGBufferMeshPass(builder, sceneExtent, resources, rayTracingScene);
			break;
		}
		default:
			throw Diagnostics::Error("GBuffer graph construction received an invalid algorithm.");
	}

	AddSkyMotionVectorPass(builder, sceneExtent, resources);
	AddLinearizeDeviceZPass(builder, sceneExtent, resources);
}
