#include "../PCH.h"
#include "RayReconstruction/RayReconstructionFramePass.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayReconstruction/RayReconstructionInputContractBuilder.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSubsystem.h"

namespace
{
	void ResetExternalProviderCommandState(void* userData) noexcept
	{
		RenderCommandContext* const commands = static_cast<RenderCommandContext*>(userData);
		if (commands != nullptr)
		{
			commands->GetRenderCommandList().ResetBoundState();
		}
	}
}

bool HasRequiredRayReconstructionProviderResources(const FrameRayReconstructionProviderResources& providerInputs) noexcept
{
	return providerInputs.NoisyInputColor.IsValid() && providerInputs.OutputColor.IsValid() &&
	       providerInputs.Depth.IsValid() && providerInputs.MotionVectors.IsValid() &&
	       providerInputs.Exposure.IsValid() && providerInputs.Normals.IsValid() &&
	       providerInputs.Roughness.IsValid() && providerInputs.DiffuseAlbedo.IsValid() &&
	       providerInputs.SpecularAlbedo.IsValid() && providerInputs.SpecularHitDistance.IsValid();
}

RayReconstructionInputContract BuildFrameRayReconstructionInputContract(
    const FrameRayReconstructionProviderResources& providerInputs,
    RenderViewportExtent sceneExtent,
    std::uint64_t frameIndex,
    const PerViewCameraConstantBufferData& camera,
    const PerTemporalConstantBufferData& temporalData,
    RenderTemporalFrameState temporalState)
{
	return BuildRayReconstructionInputContract(
	    RayReconstructionInputContractBuildDesc{
	        .NoisyInputColor = ToRenderProductHandle(providerInputs.NoisyInputColor),
	        .OutputColor = ToRenderProductHandle(providerInputs.OutputColor),
	        .Depth = ToRenderProductHandle(providerInputs.Depth),
	        .MotionVectors = ToRenderProductHandle(providerInputs.MotionVectors),
	        .Exposure = ToRenderProductHandle(providerInputs.Exposure),
	        .Normals = ToRenderProductHandle(providerInputs.Normals),
	        .Roughness = ToRenderProductHandle(providerInputs.Roughness),
	        .DiffuseAlbedo = ToRenderProductHandle(providerInputs.DiffuseAlbedo),
	        .SpecularAlbedo = ToRenderProductHandle(providerInputs.SpecularAlbedo),
	        .SpecularHitDistance = ToRenderProductHandle(providerInputs.SpecularHitDistance),
	        .RenderExtent = sceneExtent,
	        .OutputExtent = sceneExtent,
	        .FrameIndex = frameIndex,
	        .Camera = camera,
	        .TemporalData = temporalData,
	        .TemporalState = temporalState});
}

void AddRayReconstructionProviderPass(
    FrameGraphBuilder& builder,
    const char* passName,
    RenderViewportExtent sceneExtent,
    const FrameRayReconstructionProviderResources& providerInputs)
{
	builder.AddPass(
	    passName,
	    EFrameGraphPassFlags::ExternalProvider,
	    [providerInputs](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(providerInputs.NoisyInputColor, ResourceUsage::ShaderRead, "NoisyInputColor");
		    resourceBuilder.Write(providerInputs.OutputColor, ResourceUsage::UnorderedAccess, "OutputColor");
		    resourceBuilder.Read(providerInputs.Depth, ResourceUsage::ShaderRead, "Depth");
		    resourceBuilder.Read(providerInputs.MotionVectors, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Read(providerInputs.Exposure, ResourceUsage::ShaderRead, "Exposure");
		    resourceBuilder.Read(providerInputs.Normals, ResourceUsage::ShaderRead, "Normals");
		    resourceBuilder.Read(providerInputs.Roughness, ResourceUsage::ShaderRead, "Roughness");
		    resourceBuilder.Read(providerInputs.DiffuseAlbedo, ResourceUsage::ShaderRead, "DiffuseAlbedo");
		    resourceBuilder.Read(providerInputs.SpecularAlbedo, ResourceUsage::ShaderRead, "SpecularAlbedo");
		    resourceBuilder.Read(providerInputs.SpecularHitDistance, ResourceUsage::ShaderRead, "SpecularHitDistance");
	    },
	    [providerInputs, sceneExtent, passName](PassExecutionContext& context)
	    {
		    RayReconstructionEvaluationResult result{
		        .ProducedOutput = false,
		        .FailureDomain = ERayReconstructionProviderFailureDomain::Backend,
		        .Reason = "No ray reconstruction runtime service was provided."};

		    if (context.RuntimeServices.ImageProviders != nullptr &&
		        context.RuntimeServices.ImageProviders->RayReconstruction != nullptr)
		    {
			    RenderCommandList& commandList = context.Commands.GetRenderCommandList();
			    result = context.RuntimeServices.ImageProviders->RayReconstruction->Evaluate(
			        RayReconstructionEvaluationDesc{
			            .NoisyInputColor = ToRenderProductHandle(providerInputs.NoisyInputColor),
			            .OutputColor = ToRenderProductHandle(providerInputs.OutputColor),
			            .Depth = ToRenderProductHandle(providerInputs.Depth),
			            .MotionVectors = ToRenderProductHandle(providerInputs.MotionVectors),
			            .Exposure = ToRenderProductHandle(providerInputs.Exposure),
			            .Normals = ToRenderProductHandle(providerInputs.Normals),
			            .Roughness = ToRenderProductHandle(providerInputs.Roughness),
			            .DiffuseAlbedo = ToRenderProductHandle(providerInputs.DiffuseAlbedo),
			            .SpecularAlbedo = ToRenderProductHandle(providerInputs.SpecularAlbedo),
			            .SpecularHitDistance = ToRenderProductHandle(providerInputs.SpecularHitDistance),
			            .BackendApi = commandList.GetBackendApi(),
			            .NativeCommandList = commandList.GetNativeHandle(
			                RhiNativeInteropRequest{
			                    .Consumer = ERhiNativeInteropConsumer::RayReconstructionProvider,
			                    .Reason = passName}),
			            .NativeNoisyInputColor = context.Resources.ResolveResource(providerInputs.NoisyInputColor),
			            .NativeOutputColor = context.Resources.ResolveResource(providerInputs.OutputColor),
			            .NativeDepth = context.Resources.ResolveResource(providerInputs.Depth),
			            .NativeMotionVectors = context.Resources.ResolveResource(providerInputs.MotionVectors),
			            .NativeExposure = context.Resources.ResolveResource(providerInputs.Exposure),
			            .NativeNormals = context.Resources.ResolveResource(providerInputs.Normals),
			            .NativeRoughness = context.Resources.ResolveResource(providerInputs.Roughness),
			            .NativeDiffuseAlbedo = context.Resources.ResolveResource(providerInputs.DiffuseAlbedo),
			            .NativeSpecularAlbedo = context.Resources.ResolveResource(providerInputs.SpecularAlbedo),
			            .NativeSpecularHitDistance = context.Resources.ResolveResource(providerInputs.SpecularHitDistance),
			            .NativeNoisyInputColorView =
			                context.Resources.ResolveNativeTextureView(providerInputs.NoisyInputColor, ResourceState::ShaderResource),
			            .NativeOutputColorView =
			                context.Resources.ResolveNativeTextureView(providerInputs.OutputColor, ResourceState::UnorderedAccess),
			            .NativeDepthView = context.Resources.ResolveNativeTextureView(providerInputs.Depth, ResourceState::ShaderResource),
			            .NativeMotionVectorsView =
			                context.Resources.ResolveNativeTextureView(providerInputs.MotionVectors, ResourceState::ShaderResource),
			            .NativeExposureView = context.Resources.ResolveNativeTextureView(providerInputs.Exposure, ResourceState::ShaderResource),
			            .NativeNormalsView = context.Resources.ResolveNativeTextureView(providerInputs.Normals, ResourceState::ShaderResource),
			            .NativeRoughnessView = context.Resources.ResolveNativeTextureView(providerInputs.Roughness, ResourceState::ShaderResource),
			            .NativeDiffuseAlbedoView =
			                context.Resources.ResolveNativeTextureView(providerInputs.DiffuseAlbedo, ResourceState::ShaderResource),
			            .NativeSpecularAlbedoView =
			                context.Resources.ResolveNativeTextureView(providerInputs.SpecularAlbedo, ResourceState::ShaderResource),
			            .NativeSpecularHitDistanceView =
			                context.Resources.ResolveNativeTextureView(providerInputs.SpecularHitDistance, ResourceState::ShaderResource),
			            .RenderExtent = sceneExtent,
			            .OutputExtent = sceneExtent,
			            .ResetCommandState = &ResetExternalProviderCommandState,
			            .ResetCommandStateUserData = &context.Commands});
		    }

		    if (!result.ProducedOutput)
		    {
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(providerInputs.NoisyInputColor),
			        ResourceState::ShaderResource,
			        ResourceState::CopySource);
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(providerInputs.OutputColor),
			        ResourceState::UnorderedAccess,
			        ResourceState::CopyDest);
			    context.Resources.CopyTexture(context.Commands, providerInputs.OutputColor, providerInputs.NoisyInputColor);
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(providerInputs.NoisyInputColor),
			        ResourceState::CopySource,
			        ResourceState::ShaderResource);
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(providerInputs.OutputColor),
			        ResourceState::CopyDest,
			        ResourceState::UnorderedAccess);
		    }
	    });
}
