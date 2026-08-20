#include "PCH.h"
#include "Frame/GBuffer/RasterizedGBuffer.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Core/FrameAssembly.h"
#include "Passes/Deferred/GBufferPass.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <memory>

void AddRasterizedGBufferPass(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const GBufferRenderTargets& targets,
    const FrameAssemblyExternalResources& externalResources)
{
	auto& parameters = builder.AllocParameters<GBufferPass::Parameters>();
	auto* parameterFields = parameters.operator->();
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
	builder.AddPreparedSceneSetup(
	    [frameInput](const PreparedRenderScene& preparedScene) { frameInput->PreparedScene = std::cref(preparedScene); });
	builder.AddRenderViewSetup(
	    [parameterFields, frameInput](const RenderView& view)
	    {
		    frameInput->View = std::cref(view);
		    parameterFields->View = view.uniform;
		    parameterFields->ViewCamera = view.cameraUniform;
		    parameterFields->ViewTemporal = view.temporalUniform;
	    });
	builder.Draw<GBufferPass>(parameters, gpuMeshCache, frameInput);
}
