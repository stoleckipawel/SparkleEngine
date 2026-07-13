#include "PCH.h"
#include "Frame/GBuffer/RaytracedGBuffer.h"

#include "Frame/GBuffer/RaytracedGBufferTargetClear.h"
#include "Frame/Core/FrameAssembly.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/RayTracing/RaytracedGBufferPass.h"

void AddRaytracedGBufferPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    const FrameAssemblyExternalResources& externalResources)
{
	AddRaytracedGBufferTargetClearPass(builder, targets);
	auto& parameters = builder.AllocPassParameters<RaytracedGBufferPass>();
	RaytracedGBufferPass::DeclareResources(
	    builder,
	    targets,
	    sceneTlas,
	    externalResources.RayTracingHitVertices,
	    externalResources.RayTracingHitSkinInfluences,
	    externalResources.RayTracingHitIndices,
	    externalResources.RayTracingHitInstances,
	    externalResources.RayTracingHitMaterials,
	    externalResources.MeshInstances,
	    externalResources.JointMatrices,
	    externalResources.PreviousJointMatrices,
	    parameters);
	builder.AddComputeShaderPass<RaytracedGBufferPass>(parameters);
}
