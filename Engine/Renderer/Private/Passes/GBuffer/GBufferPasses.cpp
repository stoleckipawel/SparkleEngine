#include "PCH.h"
#include "Passes/GBuffer/GBufferPasses.h"

#include "Frame/RenderFrame.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Debug/RendererCVars.h"
#include "Passes/GBuffer/GBufferRenderTargets.h"
#include "Passes/GBuffer/LinearizeDeviceZ.h"
#include "Passes/GBuffer/Raster/RasterizedGBufferMesh.h"
#include "Passes/GBuffer/RayTracing/RayTracingGBufferMesh.h"
#include "Passes/GBuffer/SkyMotionVector.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"

void AddGBufferPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    RenderViewportExtent sceneExtent,
    GpuMeshCache& gpuMeshCache,
    RenderRayTracingScene& rayTracingScene,
    RenderFrameGraphResources& resources)
{
	const GBufferAlgorithm algorithm = CVarGBufferAlgorithm.Get();

	CreateGBufferRenderTargets(builder, sceneExtent, algorithm, resources);

	switch (algorithm)
	{
		case GBufferAlgorithm::Rasterized:
			AddRasterizedGBufferMeshPass(builder, frame, gpuMeshCache, resources);
			break;
		case GBufferAlgorithm::RayTracing:
			AddRayTracingGBufferMeshPass(builder, frame, sceneExtent, resources, rayTracingScene);
			break;
		default:
			throw Diagnostics::Error("GBuffer graph construction received an invalid algorithm.");
	}

	AddSkyMotionVectorPass(builder, frame, sceneExtent, resources);
	AddLinearizeDeviceZPass(builder, frame, sceneExtent, resources);
}
