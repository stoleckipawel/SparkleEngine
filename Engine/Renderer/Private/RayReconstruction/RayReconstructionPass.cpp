#include "../PCH.h"
#include "RayReconstruction/RayReconstructionPass.h"

#include "Commands/RenderCommandContext.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayReconstruction/RayReconstructionProvider.h"

namespace
{
	bool HasRequiredInputs(const RayReconstructionPassResources& inputs) noexcept
	{
		return inputs.NoisyInputColor.IsValid() && inputs.OutputColor.IsValid() && inputs.Depth.IsValid() &&
		       inputs.MotionVectors.IsValid() && inputs.Exposure.IsValid() && inputs.Normals.IsValid() &&
		       inputs.Roughness.IsValid() && inputs.DiffuseAlbedo.IsValid() && inputs.SpecularAlbedo.IsValid() &&
		       inputs.SpecularHitDistance.IsValid();
	}
}

void AddRayReconstructionPass(
    FrameGraphBuilder& builder,
    const char* passName,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent,
    const RayReconstructionPassResources& providerInputs)
{
	if (!HasRequiredInputs(providerInputs))
	{
		return;
	}

	builder.AddPass(
	    passName,
	    EFrameGraphPassFlags::ExternalProvider,
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
		    if (context.RuntimeServices.ImageProviders != nullptr && context.RuntimeServices.ImageProviders->RayReconstruction != nullptr)
		    {
			    RenderCommandList& commandList = context.Commands.GetRenderCommandList();
			    (void) context.RuntimeServices.ImageProviders->RayReconstruction->Evaluate(
			        RayReconstructionEvaluationDesc{
			            .BackendApi = commandList.GetBackendApi(),
			            .NativeCommandList = commandList.GetNativeHandle(
			                RhiNativeInteropRequest{.Consumer = ERhiNativeInteropConsumer::RayReconstructionProvider, .Reason = passName}),
			            .NativeNoisyInputColorView =
			                context.Resources.ResolveNativeTextureView(providerInputs.NoisyInputColor, ResourceState::ShaderResource),
			            .NativeOutputColorView =
			                context.Resources.ResolveNativeTextureView(providerInputs.OutputColor, ResourceState::UnorderedAccess),
			            .NativeDepthView = context.Resources.ResolveNativeTextureView(providerInputs.Depth, ResourceState::ShaderResource),
			            .NativeMotionVectorsView =
			                context.Resources.ResolveNativeTextureView(providerInputs.MotionVectors, ResourceState::ShaderResource),
			            .NativeExposureView =
			                context.Resources.ResolveNativeTextureView(providerInputs.Exposure, ResourceState::ShaderResource),
			            .NativeNormalsView =
			                context.Resources.ResolveNativeTextureView(providerInputs.Normals, ResourceState::ShaderResource),
			            .NativeRoughnessView =
			                context.Resources.ResolveNativeTextureView(providerInputs.Roughness, ResourceState::ShaderResource),
			            .NativeDiffuseAlbedoView =
			                context.Resources.ResolveNativeTextureView(providerInputs.DiffuseAlbedo, ResourceState::ShaderResource),
			            .NativeSpecularAlbedoView =
			                context.Resources.ResolveNativeTextureView(providerInputs.SpecularAlbedo, ResourceState::ShaderResource),
			            .NativeSpecularHitDistanceView =
			                context.Resources.ResolveNativeTextureView(providerInputs.SpecularHitDistance, ResourceState::ShaderResource),
			            .RenderExtent = renderExtent,
			            .OutputExtent = outputExtent});
		    }
	    });
}
