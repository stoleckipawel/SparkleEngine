#include "../../PCH.h"
#include "Frame/Lighting/IndirectReconstruction.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "RayReconstruction/RayReconstructionInputContractBuilder.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"
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

FrameRayReconstructionProviderResources BuildIndirectRayReconstructionProviderInputs(
    const SceneRenderTargets& sceneTargets,
    const GBufferRenderTargets& gbuffer,
    const LightingRenderTargets& lighting,
    FrameGraphTextureHandle exposure)
{
	return FrameRayReconstructionProviderResources{
	    .NoisyInputColor = sceneTargets.SceneColor,
	    .OutputColor = sceneTargets.FinalSceneColor,
	    .Depth = sceneTargets.MainDepth,
	    .MotionVectors = gbuffer.MotionVector,
	    .Exposure = exposure,
	    .Normals = gbuffer.Normal,
	    .IndirectReconstruction =
	        {.NoisyIndirectDiffuse = lighting.IndirectDiffuse,
	         .NoisyIndirectSpecular = lighting.IndirectSpecular,
	         .DemodulatedIndirectDiffuse = lighting.IndirectDiffuseDemodulatedRadiance,
	         .DemodulatedIndirectSpecular = lighting.IndirectSpecularDemodulatedRadiance,
	         .DiffuseAlbedo = lighting.IndirectDiffuseAlbedo,
	         .SpecularAlbedo = lighting.IndirectSpecularAlbedo,
	         .MaterialGuide = lighting.IndirectMaterialGuide,
	         .DiffuseSampleGuide = lighting.IndirectDiffuseSampleGuide,
	         .SpecularSampleGuide = lighting.IndirectSpecularSampleGuide}};
}

RayReconstructionInputContract BuildFrameIndirectRayReconstructionInputContract(
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
	        .Roughness = ToRenderProductHandle(providerInputs.IndirectReconstruction.MaterialGuide),
	        .DiffuseAlbedo = ToRenderProductHandle(providerInputs.IndirectReconstruction.DiffuseAlbedo),
	        .SpecularAlbedo = ToRenderProductHandle(providerInputs.IndirectReconstruction.SpecularAlbedo),
	        .SpecularHitDistance = ToRenderProductHandle(providerInputs.IndirectReconstruction.SpecularSampleGuide),
	        .RenderExtent = sceneExtent,
	        .OutputExtent = sceneExtent,
	        .FrameIndex = frameIndex,
	        .Camera = camera,
	        .TemporalData = temporalData,
	        .TemporalState = temporalState});
}

void AddIndirectRayReconstructionPass(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    const FrameRayReconstructionProviderResources& providerInputs)
{
	builder.AddPass(
	    "IndirectRayReconstruction",
	    EFrameGraphPassFlags::ExternalProvider,
	    [providerInputs](PassResourceBuilder& resourceBuilder)
	    {
		    resourceBuilder.Read(providerInputs.NoisyInputColor, ResourceUsage::CopySource, "NoisyInputColor");
		    resourceBuilder.Write(providerInputs.OutputColor, ResourceUsage::UnorderedAccess, "OutputColor");
		    resourceBuilder.Read(providerInputs.Depth, ResourceUsage::DepthRead, "Depth");
		    resourceBuilder.Read(providerInputs.MotionVectors, ResourceUsage::ShaderRead, "MotionVectors");
		    resourceBuilder.Read(providerInputs.Exposure, ResourceUsage::ShaderRead, "Exposure");
		    resourceBuilder.Read(providerInputs.Normals, ResourceUsage::ShaderRead, "Normals");
		    resourceBuilder.Read(providerInputs.IndirectReconstruction.MaterialGuide, ResourceUsage::ShaderRead, "Roughness");
		    resourceBuilder.Read(providerInputs.IndirectReconstruction.DiffuseAlbedo, ResourceUsage::ShaderRead, "DiffuseAlbedo");
		    resourceBuilder.Read(providerInputs.IndirectReconstruction.SpecularAlbedo, ResourceUsage::ShaderRead, "SpecularAlbedo");
		    resourceBuilder.Read(providerInputs.IndirectReconstruction.SpecularSampleGuide, ResourceUsage::ShaderRead, "SpecularHitDistance");
	    },
	    [providerInputs, sceneExtent](PassExecutionContext& context)
	    {
		    RayReconstructionEvaluationResult result{
		        .ProducedOutput = false,
		        .UsedFallback = true,
		        .FailureDomain = ERayReconstructionProviderFailureDomain::Backend,
		        .Reason = "No ray reconstruction runtime service was provided."};

		    if (context.RuntimeServices.ImageProviders != nullptr &&
		        context.RuntimeServices.ImageProviders->RayReconstruction.Subsystem != nullptr)
		    {
			    RenderCommandList& commandList = context.Commands.GetRenderCommandList();
			    result = context.RuntimeServices.ImageProviders->RayReconstruction.Subsystem->Evaluate(
			        RayReconstructionEvaluationDesc{
			            .NoisyInputColor = ToRenderProductHandle(providerInputs.NoisyInputColor),
			            .OutputColor = ToRenderProductHandle(providerInputs.OutputColor),
			            .Depth = ToRenderProductHandle(providerInputs.Depth),
			            .MotionVectors = ToRenderProductHandle(providerInputs.MotionVectors),
			            .Exposure = ToRenderProductHandle(providerInputs.Exposure),
			            .Normals = ToRenderProductHandle(providerInputs.Normals),
			            .Roughness = ToRenderProductHandle(providerInputs.IndirectReconstruction.MaterialGuide),
			            .DiffuseAlbedo = ToRenderProductHandle(providerInputs.IndirectReconstruction.DiffuseAlbedo),
			            .SpecularAlbedo = ToRenderProductHandle(providerInputs.IndirectReconstruction.SpecularAlbedo),
			            .SpecularHitDistance = ToRenderProductHandle(providerInputs.IndirectReconstruction.SpecularSampleGuide),
			            .BackendApi = commandList.GetBackendApi(),
			            .NativeCommandList = commandList.GetNativeHandle(
			                RhiNativeInteropRequest{
			                    .Consumer = ERhiNativeInteropConsumer::RayReconstructionProvider,
			                    .Reason = "Evaluate indirect ray reconstruction pass"}),
			            .NativeNoisyInputColor = context.Resources.ResolveResource(providerInputs.NoisyInputColor),
			            .NativeOutputColor = context.Resources.ResolveResource(providerInputs.OutputColor),
			            .NativeDepth = context.Resources.ResolveResource(providerInputs.Depth),
			            .NativeMotionVectors = context.Resources.ResolveResource(providerInputs.MotionVectors),
			            .NativeExposure = context.Resources.ResolveResource(providerInputs.Exposure),
			            .NativeNormals = context.Resources.ResolveResource(providerInputs.Normals),
			            .NativeRoughness = context.Resources.ResolveResource(providerInputs.IndirectReconstruction.MaterialGuide),
			            .NativeDiffuseAlbedo = context.Resources.ResolveResource(providerInputs.IndirectReconstruction.DiffuseAlbedo),
			            .NativeSpecularAlbedo = context.Resources.ResolveResource(providerInputs.IndirectReconstruction.SpecularAlbedo),
			            .NativeSpecularHitDistance = context.Resources.ResolveResource(providerInputs.IndirectReconstruction.SpecularSampleGuide),
			            .NativeNoisyInputColorView =
			                context.Resources.ResolveNativeTextureView(providerInputs.NoisyInputColor, ResourceState::CopySource),
			            .NativeOutputColorView =
			                context.Resources.ResolveNativeTextureView(providerInputs.OutputColor, ResourceState::UnorderedAccess),
			            .NativeDepthView = context.Resources.ResolveNativeTextureView(providerInputs.Depth, ResourceState::DepthRead),
			            .NativeMotionVectorsView =
			                context.Resources.ResolveNativeTextureView(providerInputs.MotionVectors, ResourceState::ShaderResource),
			            .NativeExposureView = context.Resources.ResolveNativeTextureView(providerInputs.Exposure, ResourceState::ShaderResource),
			            .NativeNormalsView = context.Resources.ResolveNativeTextureView(providerInputs.Normals, ResourceState::ShaderResource),
			            .NativeRoughnessView = context.Resources.ResolveNativeTextureView(
			                providerInputs.IndirectReconstruction.MaterialGuide,
			                ResourceState::ShaderResource),
			            .NativeDiffuseAlbedoView = context.Resources.ResolveNativeTextureView(
			                providerInputs.IndirectReconstruction.DiffuseAlbedo,
			                ResourceState::ShaderResource),
			            .NativeSpecularAlbedoView = context.Resources.ResolveNativeTextureView(
			                providerInputs.IndirectReconstruction.SpecularAlbedo,
			                ResourceState::ShaderResource),
			            .NativeSpecularHitDistanceView = context.Resources.ResolveNativeTextureView(
			                providerInputs.IndirectReconstruction.SpecularSampleGuide,
			                ResourceState::ShaderResource),
			            .RenderExtent = sceneExtent,
			            .OutputExtent = sceneExtent,
			            .ResetCommandState = &ResetExternalProviderCommandState,
			            .ResetCommandStateUserData = &context.Commands});
		    }

		    if (!result.ProducedOutput || result.UsedFallback)
		    {
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(providerInputs.OutputColor),
			        ResourceState::UnorderedAccess,
			        ResourceState::CopyDest);
			    context.Resources.CopyTexture(context.Commands, providerInputs.OutputColor, providerInputs.NoisyInputColor);
			    context.Commands.TransitionResource(
			        context.Resources.ResolveResource(providerInputs.OutputColor),
			        ResourceState::CopyDest,
			        ResourceState::UnorderedAccess);
		    }
	    });
}

bool AddIndirectRayReconstructionPassIfEnabled(
    FrameGraphBuilder& builder,
    RenderViewportExtent sceneExtent,
    FrameAssemblyResourceLayout& resources)
{
	if (GetRayReconstructionModeFromCVars() != EngineRayReconstructionMode::NvidiaDlssRayReconstruction)
	{
		return false;
	}

	resources.RayReconstructionProviderInputs = BuildIndirectRayReconstructionProviderInputs(
	    resources.Transient.Scene,
	    resources.Transient.GBuffer,
	    resources.Transient.Lighting,
	    resources.Transient.Exposure);
	AddIndirectRayReconstructionPass(builder, sceneExtent, resources.RayReconstructionProviderInputs);
	resources.FinalSceneColorProduced = true;
	return true;
}
