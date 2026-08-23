#include "PCH.h"
#include "Passes/GBuffer/RasterizedGBuffer.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/GBuffer/GBufferPass.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <memory>

void AddRasterizedGBufferPass(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const GBufferRenderTargets& targets,
    const RenderFrameGraphImportedSceneResources& externalResources)
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
	parameters->MeshInstanceSlots = builder.CreateSRV<std::uint32_t>(externalResources.Scene.Geometry.MeshInstanceSlots);
	parameters->JointMatrices = builder.CreateSRV<JointMatrixData>(externalResources.Scene.Geometry.JointMatrices);
	parameters->PreviousJointMatrices = builder.CreateSRV<JointMatrixData>(externalResources.Scene.Geometry.PreviousJointMatrices);
	parameters->MorphWeights = builder.CreateSRV<float>(externalResources.Scene.Geometry.MorphWeights);
	parameters->PreviousMorphWeights = builder.CreateSRV<float>(externalResources.Scene.Geometry.PreviousMorphWeights);
	parameters->SamplerAniso16xWrap = RhiSamplerDesc{.MaxAnisotropy = RhiSamplerAnisotropy::X16};
	auto frameInput = std::make_shared<GBufferPassFrameInput>();
	builder.AddParameterSetup<PreparedRenderScene>(
	    [frameInput](const PreparedRenderScene& preparedScene) { frameInput->PreparedScene = std::cref(preparedScene); });
	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [frameInput](auto& fields, const RenderView& view)
	    {
		    frameInput->View = std::cref(view);
		    fields.View = view.uniform;
		    fields.ViewCamera = view.cameraUniform;
		    fields.ViewTemporal = view.temporalUniform;
	    });
	builder.Draw<GBufferPass>(parameters, gpuMeshCache, frameInput);
}
