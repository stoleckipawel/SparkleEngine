#include "PCH.h"
#include "Passes/GBuffer/RasterizedGBuffer.h"

#include "Config/DepthConvention.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/GBuffer/GBufferMeshPass.h"
#include "Passes/GBuffer/GBufferShaders.h"
#include "Pipeline/RasterPassRenderState.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <cstdint>
#include <functional>
#include <memory>

void AddRasterizedGBufferMeshPass(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const GBufferRenderTargets& targets,
    const RenderFrameGraphImportedSceneResources& externalResources)
{
	auto& parameters = builder.AllocGraphParameters<GBufferGraphParameters>("GBuffer");
	parameters->BaseColor =
	    builder.CreateRenderTarget(targets.BaseColor, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
	parameters->Normal =
	    builder.CreateRenderTarget(targets.Normal, FrameGraphAttachmentLoadAction::Clear, FrameGraphAttachmentStoreAction::Store);
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
	auto frameInput = std::make_shared<GBufferMeshPassInput>();
	builder.AddParameterSetup<PreparedRenderScene>(
	    [frameInput](const PreparedRenderScene& preparedScene) { frameInput->PreparedScene = std::cref(preparedScene); });
	builder.AddParameterSetup<RenderView>(
	    parameters,
	    [frameInput](auto& fields, const RenderView& view)
	    {
		    frameInput->View = std::cref(view);
		    frameInput->Viewport = view.viewport;
		    frameInput->Scissor = view.scissorRect;
		    frameInput->Wireframe = view.uniform.ViewModeIndex == static_cast<std::uint32_t>(RenderViewMode::Wireframe);
		    fields.Shader.Vertex.ViewCamera = view.cameraUniform;
		    fields.Shader.Vertex.ViewTemporal = view.temporalUniform;
		    fields.Shader.Pixel.View = view.uniform;
		    fields.Shader.Pixel.ViewTemporal = view.temporalUniform;
	    });
	RasterPassRenderState renderState;
	renderState.SetOpaqueBlend();
	renderState.SetDepthTest(DepthConvention::GetDepthComparisonLessEqualFunc());
	renderState.SetDepthWrite(true);
	renderState.DisableStencil();
	builder.Draw<GBufferVS, GBufferPS>(parameters, renderState, GBufferMeshPass(gpuMeshCache, frameInput));
}
