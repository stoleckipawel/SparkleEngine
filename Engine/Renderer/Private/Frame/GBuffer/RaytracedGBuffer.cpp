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
	parameters->RayTracingHitVertices = builder.CreateSRV(externalResources.Scene.RayTracing.Vertices);
	parameters->SkinInfluences = builder.CreateSRV(externalResources.Scene.RayTracing.SkinInfluences);
	parameters->RayTracingHitIndices = builder.CreateSRV(externalResources.Scene.RayTracing.Indices);
	parameters->RayTracingHitInstances = builder.CreateSRV(externalResources.Scene.RayTracing.Instances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(externalResources.Scene.RayTracing.Materials);
	parameters->MeshInstances = builder.CreateSRV(externalResources.Scene.Geometry.MeshInstances);
	parameters->JointMatrices = builder.CreateSRV(externalResources.Scene.Geometry.JointMatrices);
	parameters->PreviousJointMatrices = builder.CreateSRV(externalResources.Scene.Geometry.PreviousJointMatrices);
	builder.Dispatch<RaytracedGBufferPass>(parameters);
}
