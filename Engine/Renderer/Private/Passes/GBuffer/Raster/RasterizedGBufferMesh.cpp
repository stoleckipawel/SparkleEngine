#include "PCH.h"
#include "Passes/GBuffer/Raster/RasterizedGBufferMesh.h"

#include "Frame/RenderFrame.h"
#include "Config/DepthConvention.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/GBuffer/Raster/GBufferMeshPass.h"
#include "Passes/GBuffer/Raster/GBufferShaders.h"
#include "Pipeline/RasterPassRenderState.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <cstdint>

void AddRasterizedGBufferMeshPass(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    GpuMeshCache& gpuMeshCache,
    const RenderFrameGraphResources& resources)
{
	const GBufferRenderTargets& targets = resources.Transient.GBuffer;
	const RenderFrameGraphImportedSceneResources& externalResources = resources.ImportedScene;

	auto& parameters = builder.AllocGraphParameters<GBufferGraphParameters>("GBuffer");
	parameters->BaseColor =
	    builder.CreateRenderTarget(targets.BaseColor, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->WorldNormal =
	    builder.CreateRenderTarget(targets.WorldNormal, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->WorldTangent =
	    builder.CreateRenderTarget(targets.WorldTangent, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->Material =
	    builder.CreateRenderTarget(targets.Material, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->Emissive =
	    builder.CreateRenderTarget(targets.Emissive, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->Subsurface =
	    builder.CreateRenderTarget(targets.Subsurface, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->MotionVector =
	    builder.CreateRenderTarget(targets.MotionVector, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->DeviceZ = builder.CreateDepthTarget(
	    targets.DeviceZ,
	    FrameGraphAttachmentLoadAction::Clear,
	    FrameGraphAttachmentStoreAction::Store,
	    FrameGraphDepthStencilAccess::ReadWrite);

	parameters->Shader.Vertex.MeshInstances = builder.CreateSRV<MeshInstanceData>(externalResources.Scene.Geometry.MeshInstances);
	parameters->Shader.Vertex.MeshInstanceSlots = builder.CreateSRV<std::uint32_t>(externalResources.Scene.Geometry.MeshInstanceSlots);
	parameters->Shader.Vertex.JointMatrices = builder.CreateSRV<JointMatrixData>(externalResources.Scene.Geometry.JointMatrices);
	parameters->Shader.Vertex.PreviousJointMatrices =
	    builder.CreateSRV<JointMatrixData>(externalResources.Scene.Geometry.PreviousJointMatrices);
	parameters->Shader.Vertex.MorphWeights = builder.CreateSRV<float>(externalResources.Scene.Geometry.MorphWeights);
	parameters->Shader.Vertex.PreviousMorphWeights = builder.CreateSRV<float>(externalResources.Scene.Geometry.PreviousMorphWeights);
	parameters->Shader.Pixel.SamplerAniso16xWrap = RhiSamplerDesc{.MaxAnisotropy = RhiSamplerAnisotropy::X16};

	parameters->Shader.Vertex.ViewCamera = frame.View.cameraUniform;
	parameters->Shader.Vertex.ViewTemporal = frame.View.temporalUniform;
	parameters->Shader.Pixel.View = frame.View.uniform;
	parameters->Shader.Pixel.ViewTemporal = frame.View.temporalUniform;

	RasterPassRenderState renderState;
	renderState.SetOpaqueBlend();
	renderState.SetDepthTest(DepthConvention::GetDepthComparisonLessEqualFunc());
	renderState.SetDepthWrite(true);
	renderState.DisableStencil();

	builder.Draw<GBufferVS, GBufferPS>(parameters, renderState, GBufferMeshPass(gpuMeshCache, frame.PreparedScene, frame.View));
}
