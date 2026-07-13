#include "../../PCH.h"
#include "Passes/RayTracing/RaytracedGBufferPass.h"

#include "Frame/Core/FrameContext.h"
#include "Frame/Core/RenderViewData.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassExecutionContext.h"
#include "FrameGraph/PassRuntimeServices.h"
#include "Passes/Core/ComputePassUtilities.h"
#include "Passes/Core/RenderPassDefinition.h"
#include "RayTracing/RayTracingShaderFeatureFlags.h"
#include "Pipeline/PassPipelineRuntime.h"
#include "RayTracing/RayTracingPassCapabilityQuery.h"
#include "Renderer/ShaderRegistrations/RendererShaderPackages.h"
#include "RHI/Public/Samplers/RhiSamplerDesc.h"
#include "SceneData/MaterialTextureTableCapability.h"

RaytracedGBufferPass::RaytracedGBufferPass(const ComputePassPipelineRuntime& runtime) noexcept : m_runtime(runtime) {}

const RaytracedGBufferPass::ParameterMetadata& RaytracedGBufferPass::GetParameterMetadata() noexcept
{
	return ComputePassUtilities::BuildParameterMetadata<RaytracedGBufferPass>();
}

const RenderPassDefinition& RaytracedGBufferPass::GetDefinition() noexcept
{
	static const RenderPassDefinition definition = ComputePassUtilities::BuildDefinition(
	    PassName,
	    RendererShaderPackages::RaytracedGBuffer,
	    L"RaytracedGBuffer_BindingLayout",
	    L"RaytracedGBuffer_PipelineState",
	    RayTracingShaderFeatureFlags::DescriptorRayQuery);
	return definition;
}

void RaytracedGBufferPass::DeclareResources(
    FrameGraphBuilder& builder,
    const GBufferRenderTargets& targets,
    FrameGraphAccelerationStructureHandle sceneTlas,
    FrameGraphBufferHandle hitVertices,
    FrameGraphBufferHandle hitSkinInfluences,
    FrameGraphBufferHandle hitIndices,
    FrameGraphBufferHandle hitInstances,
    FrameGraphBufferHandle hitMaterials,
    FrameGraphBufferHandle meshInstances,
    FrameGraphBufferHandle jointMatrices,
    FrameGraphBufferHandle previousJointMatrices,
    ParameterInstance& parameters)
{
	parameters->GBufferBaseColor = builder.CreateUAV(targets.BaseColor);
	parameters->GBufferNormal = builder.CreateUAV(targets.Normal);
	parameters->GBufferMaterial = builder.CreateUAV(targets.Material);
	parameters->GBufferEmissive = builder.CreateUAV(targets.Emissive);
	parameters->GBufferSubsurface = builder.CreateUAV(targets.Subsurface);
	parameters->GBufferDeviceZ = builder.CreateUAV(targets.DeviceZ);
	parameters->GBufferMotionVector = builder.CreateUAV(targets.MotionVector);
	parameters->SceneTlas = builder.Read(sceneTlas);
	parameters->RayTracingHitVertices = builder.CreateSRV(hitVertices);
	parameters->SkinInfluences = builder.CreateSRV(hitSkinInfluences);
	parameters->RayTracingHitIndices = builder.CreateSRV(hitIndices);
	parameters->RayTracingHitInstances = builder.CreateSRV(hitInstances);
	parameters->RayTracingHitMaterials = builder.CreateSRV(hitMaterials);
	parameters->MeshInstances = builder.CreateSRV(meshInstances);
	parameters->JointMatrices = builder.CreateSRV(jointMatrices);
	parameters->PreviousJointMatrices = builder.CreateSRV(previousJointMatrices);
}

void RaytracedGBufferPass::SetParameters(
    ParameterInstance& parameters,
    const FrameContext& frame,
    const RenderViewData& viewData,
    const PassRuntimeServices& passRuntimeServices) const
{
	parameters->PerFrame = passRuntimeServices.PerFrame;
	parameters->PerView = viewData.perViewData;
	parameters->PerTemporal = viewData.perTemporalData;
	parameters->MaterialTextureSampler = RhiSamplerDesc{
	    .MinMagFilter = RhiSamplerMinMagFilter::Linear,
	    .MipFilter = RhiSamplerMipFilter::Linear,
	    .Address = MakeRhiSamplerAddressModes(RhiSamplerAddressMode::Wrap),
	    .MaxAnisotropy = RhiSamplerAnisotropy::X1};
}

void RaytracedGBufferPass::Execute(PassExecutionContext& context, ParameterInstance& parameters) const
{
	const RayTracingPassCapabilities rayTracingCapabilities =
	    RayTracingPassCapabilityQuery::Build(context.Frame, context.RuntimeServices.RayTracing);
	if (!rayTracingCapabilities.InlineRayQueryAvailable ||
	    !RayTracingPassCapabilityQuery::CanUseSceneTlas(rayTracingCapabilities, RayTracingSceneTlasShaderAccessMode::Descriptor))
	{
		return;
	}
	if (!rayTracingCapabilities.HitDataAvailable || !rayTracingCapabilities.MaterialTextureTableAvailable)
	{
		return;
	}

	const RenderBindingSet* materialTextureTable = context.Frame.sceneData.materialTextureTable;
	const std::uint32_t descriptorCount = context.Frame.sceneData.materialTextureTableDescriptorCount;
	if (!context.Frame.sceneData.materialTextureTableValid || materialTextureTable == nullptr || !*materialTextureTable ||
	    descriptorCount == 0u || descriptorCount > MaterialTextureTableFixedCapacity ||
	    materialTextureTable->GetDescriptorCount() < descriptorCount)
	{
		return;
	}
	parameters->MaterialTextureTable = materialTextureTable->GetTableBinding(0);

	SetParameters(parameters, context.Frame, context.Frame.mainView, context.RuntimeServices);
	parameters->RaytracedGBufferConstants = RaytracedGBufferUniformData{
	    .RayTracingHitDataAvailable = rayTracingCapabilities.HitDataAvailable ? 1u : 0u,
	    .RayTracingHitInstanceCount = context.Frame.rayTracingHitData.GetInstanceCount(),
	    .RayTracingHitMaterialCount = context.Frame.rayTracingHitData.GetMaterialCount()};
	ComputePassUtilities::DispatchSized<RaytracedGBufferPass>(
	    context,
	    m_runtime,
	    parameters,
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Width),
	    static_cast<std::uint32_t>(context.Frame.mainView.viewport.Height));
}
