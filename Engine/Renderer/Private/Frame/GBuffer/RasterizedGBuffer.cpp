#include "PCH.h"
#include "Frame/GBuffer/RasterizedGBuffer.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Core/FrameAssembly.h"
#include "Passes/Deferred/GBufferPass.h"

void AddRasterizedGBufferPass(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& parameters = builder.AllocParameters<GBufferPass::Parameters>();
	parameters->BaseColor = builder.CreateRenderTarget(targets.BaseColor);
	parameters->Normal = builder.CreateRenderTarget(targets.Normal);
	parameters->Material = builder.CreateRenderTarget(targets.Material);
	parameters->Emissive = builder.CreateRenderTarget(targets.Emissive);
	parameters->Subsurface = builder.CreateRenderTarget(targets.Subsurface);
	parameters->MotionVector = builder.CreateRenderTarget(targets.MotionVector);
	parameters->DeviceZ = builder.CreateDepthTarget(targets.DeviceZ);
	parameters->MeshInstances = builder.CreateSRV<MeshInstanceData>(externalResources.Scene.Geometry.MeshInstances);
	parameters->JointMatrices = builder.CreateSRV<JointMatrixData>(externalResources.Scene.Geometry.JointMatrices);
	parameters->PreviousJointMatrices = builder.CreateSRV<JointMatrixData>(externalResources.Scene.Geometry.PreviousJointMatrices);
	builder.Draw<GBufferPass>(parameters);
}
