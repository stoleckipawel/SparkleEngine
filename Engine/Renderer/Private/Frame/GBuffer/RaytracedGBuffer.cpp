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
	auto& parameters = builder.AllocParameters<RaytracedGBufferPass::Parameters>();
	parameters->GBufferBaseColor = builder.CreateUAV(targets.BaseColor);
	parameters->GBufferNormal = builder.CreateUAV(targets.Normal);
	parameters->GBufferMaterial = builder.CreateUAV(targets.Material);
	parameters->GBufferEmissive = builder.CreateUAV(targets.Emissive);
	parameters->GBufferSubsurface = builder.CreateUAV(targets.Subsurface);
	parameters->GBufferDeviceZ = builder.CreateUAV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	parameters->SceneTlas = builder.Read(sceneTlas);
	parameters->RayTracingHitVertices = builder.CreateSRV(externalResources.RayTracingHitVertices);
	parameters->SkinInfluences = builder.CreateSRV(externalResources.RayTracingHitSkinInfluences);
	parameters->RayTracingHitIndices = builder.CreateSRV(externalResources.RayTracingHitIndices);
	parameters->RayTracingHitInstances = builder.CreateSRV(externalResources.RayTracingHitInstances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.RayTracingHitMaterials);
	parameters->MeshInstances = builder.CreateSRV(externalResources.MeshInstances);
	parameters->JointMatrices = builder.CreateSRV(externalResources.JointMatrices);
	parameters->PreviousJointMatrices = builder.CreateSRV(externalResources.PreviousJointMatrices);
	builder.Dispatch<RaytracedGBufferPass>(parameters);
}
