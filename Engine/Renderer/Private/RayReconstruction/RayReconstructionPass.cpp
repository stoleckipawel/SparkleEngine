#include "../PCH.h"
#include "RayReconstruction/RayReconstructionPass.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeContext.h"
#include "RayReconstruction/RayReconstructionProvider.h"

static const auto g_rayReconstructionPassLogger = Logging::GetOrCreateLogger("Renderer.RayReconstructionPass");

void AddRayReconstructionPass(
    FrameGraphBuilder& builder,
    const char* passName,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    const RayReconstructionPassResources& providerInputs)
{
	if (!providerInputs.NoisyInputColor.IsValid() || !providerInputs.OutputColor.IsValid() || !providerInputs.Depth.IsValid() ||
	    !providerInputs.MotionVectors.IsValid() || !providerInputs.Exposure.IsValid() || !providerInputs.Normals.IsValid() ||
	    !providerInputs.Roughness.IsValid() || !providerInputs.DiffuseAlbedo.IsValid() || !providerInputs.SpecularAlbedo.IsValid() ||
	    !providerInputs.SpecularHitDistance.IsValid())
	{
		Diagnostics::Fatal(
		    g_rayReconstructionPassLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-reconstruction pass received an incomplete resource set.");
	}

	builder.AddPass(
	    passName,
	    EFrameGraphPassKind::ExternalProvider,
	    [providerInputs](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(providerInputs.NoisyInputColor, ResourceUsage::ShaderRead, "NoisyInputColor");
		    resourceBuilder.Use(providerInputs.OutputColor, ResourceUsage::UnorderedAccess, "OutputColor");
		    resourceBuilder.Read(providerInputs.Depth, ResourceUsage::ShaderRead, "Depth");
		    resourceBuilder.Read(providerInputs.MotionVectors, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Read(providerInputs.Exposure, ResourceUsage::ShaderRead, "Exposure");
		    resourceBuilder.Read(providerInputs.Normals, ResourceUsage::ShaderRead, "Normals");
		    resourceBuilder.Read(providerInputs.Roughness, ResourceUsage::ShaderRead, "Roughness");
		    resourceBuilder.Read(providerInputs.DiffuseAlbedo, ResourceUsage::ShaderRead, "DiffuseAlbedo");
		    resourceBuilder.Read(providerInputs.SpecularAlbedo, ResourceUsage::ShaderRead, "SpecularAlbedo");
		    resourceBuilder.Read(providerInputs.SpecularHitDistance, ResourceUsage::ShaderRead, "SpecularHitDistance");
	    },
	    [providerInputs, renderExtent, outputExtent, passName](PassExecutionContext& context)
	    {
		    if (context.Runtime.ImageProviders == nullptr || context.Runtime.ImageProviders->RayReconstruction == nullptr)
		    {
			    Diagnostics::Fatal(
			        g_rayReconstructionPassLogger,
			        __FILE__,
			        __LINE__,
			        "Ray-reconstruction pass has no configured image provider.");
		    }

		    RenderCommandList& commandList = context.Commands.GetRenderCommandList();
		    const RhiNativeInteropRequest interopRequest{
		        .Consumer = ERhiNativeInteropConsumer::ExternalProvider,
		        .Reason = passName};
		    if (!context.Runtime.ImageProviders->RayReconstruction->Evaluate(
		        RayReconstructionEvaluationDesc{
			            .BackendApi = commandList.GetBackendApi(),
			            .NativeCommandList = commandList.GetNativeHandle(interopRequest),
			            .NativeNoisyInputColorView =
			                context.Resources.ResolveNativeTextureView(
			                    providerInputs.NoisyInputColor,
			                    ResourceState::ShaderResource,
			                    interopRequest),
			            .NativeOutputColorView =
			                context.Resources.ResolveNativeTextureView(
			                    providerInputs.OutputColor,
			                    ResourceState::UnorderedAccess,
			                    interopRequest),
			            .NativeDepthView =
			                context.Resources.ResolveNativeTextureView(providerInputs.Depth, ResourceState::ShaderResource, interopRequest),
			            .NativeMotionVectorsView =
			                context.Resources.ResolveNativeTextureView(
			                    providerInputs.MotionVectors,
			                    ResourceState::ShaderResource,
			                    interopRequest),
			            .NativeExposureView =
			                context.Resources.ResolveNativeTextureView(providerInputs.Exposure, ResourceState::ShaderResource, interopRequest),
			            .NativeNormalsView =
			                context.Resources.ResolveNativeTextureView(providerInputs.Normals, ResourceState::ShaderResource, interopRequest),
			            .NativeRoughnessView =
			                context.Resources.ResolveNativeTextureView(providerInputs.Roughness, ResourceState::ShaderResource, interopRequest),
			            .NativeDiffuseAlbedoView =
			                context.Resources.ResolveNativeTextureView(
			                    providerInputs.DiffuseAlbedo,
			                    ResourceState::ShaderResource,
			                    interopRequest),
			            .NativeSpecularAlbedoView =
			                context.Resources.ResolveNativeTextureView(
			                    providerInputs.SpecularAlbedo,
			                    ResourceState::ShaderResource,
			                    interopRequest),
			            .NativeSpecularHitDistanceView =
			                context.Resources.ResolveNativeTextureView(
			                    providerInputs.SpecularHitDistance,
			                    ResourceState::ShaderResource,
			                    interopRequest),
			            .RenderExtent = renderExtent,
			            .OutputExtent = outputExtent}))
		    {
			    Diagnostics::Fatal(
			        g_rayReconstructionPassLogger,
			        __FILE__,
			        __LINE__,
			        "The configured ray-reconstruction provider failed to evaluate the active frame.");
		    }
	    });
}
