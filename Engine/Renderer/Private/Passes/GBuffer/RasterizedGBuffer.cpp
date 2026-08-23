#include "PCH.h"
#include "Passes/GBuffer/RasterizedGBuffer.h"

#include "Config/DepthConvention.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Frame/Graph/RenderFrameGraphResources.h"
#include "Passes/GBuffer/GBufferFormats.h"
#include "Passes/GBuffer/GBufferMeshPass.h"
#include "Passes/GBuffer/GBufferShaders.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

#include <functional>
#include <memory>

void AddRasterizedGBufferMeshPass(
    FrameGraphBuilder& builder,
    GpuMeshCache& gpuMeshCache,
    const GBufferRenderTargets& targets,
    const RenderFrameGraphImportedSceneResources& externalResources)
{
	auto& parameters = builder.AllocGraphParameters<GBufferGraphParameters>("GBuffer");
	parameters->BaseColor = builder.CreateRenderTarget(targets.BaseColor);
	parameters->Normal = builder.CreateRenderTarget(targets.Normal);
	parameters->Material = builder.CreateRenderTarget(targets.Material);
	parameters->Emissive = builder.CreateRenderTarget(targets.Emissive);
	parameters->Subsurface = builder.CreateRenderTarget(targets.Subsurface);
	parameters->MotionVector = builder.CreateRenderTarget(targets.MotionVector);
	parameters->DeviceZ = builder.CreateDepthTarget(targets.DeviceZ);
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
		    fields.Shader.Vertex.ViewCamera = view.cameraUniform;
		    fields.Shader.Vertex.ViewTemporal = view.temporalUniform;
		    fields.Shader.Pixel.View = view.uniform;
		    fields.Shader.Pixel.ViewTemporal = view.temporalUniform;
	    });
	const GraphicsShaderPipelineState pipelineState{
	    .VertexLayout = RhiVertexLayoutKind::StaticMesh,
	    .DepthTest =
	        RhiDepthTestDesc{
	            .DepthEnable = true,
	            .DepthWriteEnable = true,
	            .DepthFunc = DepthConvention::GetDepthComparisonLessEqualFunc()},
	    .RenderTargetFormats =
	        {GBufferFormats::BaseColor,
	            GBufferFormats::Normal,
	            GBufferFormats::Material,
	            GBufferFormats::Emissive,
	            GBufferFormats::Subsurface,
	            GBufferFormats::MotionVector},
	    .RenderTargetCount = 6,
	    .DepthStencilFormat = GBufferFormats::RasterizedDeviceZ};
	builder.Draw<GBufferVS, GBufferPS>(parameters, pipelineState, GBufferMeshPass(gpuMeshCache, frameInput));
}
