#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingClassicTlasBuilder.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Math/MathUtils.h"
#include "Meshes/GpuMesh.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/RayTracing/RhiRayTracingTransformPacking.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "Scene/RayTracing/RayTracingShaderTablePlan.h"

#include <algorithm>
#include <unordered_set>

static const auto g_rayTracingClassicTlasBuilderLogger = Logging::GetOrCreateLogger("Renderer.RayTracing.ClassicTlasBuilder");

struct RayTracingClassicTlasBuilder::BuildState final
{
	std::vector<RhiRayTracingInstanceDesc> Instances;
	std::unordered_set<void*> BuiltBlasResources;
	RhiRayTracingAccelerationStructurePrebuildInfo PrebuildInfo = {};
	ERhiClassicTlasBuildFlags RequestedFlags = ERhiClassicTlasBuildFlags::None;
	ERhiClassicTlasBuildMode Mode = ERhiClassicTlasBuildMode::Build;
	const char* EventName = "Classic TLAS Build";
};

bool RayTracingClassicTlasBuilder::TlasHandle::IsValid() const noexcept
{
	return resource && gpuAddress != 0;
}

std::uint64_t RayTracingClassicTlasBuilder::AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept
{
	return alignment > 0 ? MathUtils::AlignUp(sizeInBytes, alignment) : sizeInBytes;
}

bool RayTracingClassicTlasBuilder::SupportsClassicTlasRefit(RenderHardwareInterface& renderHardwareInterface) noexcept
{
	return renderHardwareInterface.GetCapabilities().RayTracing.Groups.ClassicTlas.SupportsClassicTlasUpdate;
}

ERhiClassicTlasBuildFlags RayTracingClassicTlasBuilder::ResolveClassicTlasBuildFlags(
    RenderHardwareInterface& renderHardwareInterface) noexcept
{
	return CVarRayTracingClassicTlasRefit.Get() && SupportsClassicTlasRefit(renderHardwareInterface)
	    ? ERhiClassicTlasBuildFlags::AllowUpdate
	    : ERhiClassicTlasBuildFlags::None;
}

std::uint64_t RayTracingClassicTlasBuilder::ResolveScratchSize(
    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
    ERhiClassicTlasBuildFlags buildFlags) noexcept
{
	if (!HasFlag(buildFlags, ERhiClassicTlasBuildFlags::AllowUpdate))
	{
		return prebuildInfo.ScratchDataSizeInBytes;
	}
	return (std::max) (prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes);
}

RhiRayTracingInstanceFlags RayTracingClassicTlasBuilder::ResolveInstanceFlags(
    const PreparedRenderScene& preparedScene,
    const MeshDraw& draw) noexcept
{
	RhiRayTracingInstanceFlags flags = RhiRayTracingInstanceFlags::None;
	if (draw.MaterialSlot >= preparedScene.materials.size())
	{
		Diagnostics::Fatal(
		    g_rayTracingClassicTlasBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Classic TLAS input references a material outside the render scene.");
	}
	const MaterialData& material = preparedScene.materials[draw.MaterialSlot];
	if (material.doubleSided)
	{
		flags = flags | RhiRayTracingInstanceFlags::TriangleFacingCullDisable;
	}
	if (material.alphaMode == 1u)
	{
		flags = flags | RhiRayTracingInstanceFlags::ForceNonOpaque;
	}
	return flags;
}

RayTracingClassicTlasBuilder::RayTracingClassicTlasBuilder(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface)
{
}

RayTracingClassicTlasBuilder::~RayTracingClassicTlasBuilder() noexcept
{
	Clear();
}

void RayTracingClassicTlasBuilder::Prepare(std::uint32_t instanceCapacity) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		Diagnostics::Fatal(
		    g_rayTracingClassicTlasBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Classic TLAS builder has no render hardware interface.");
	}

	const ERhiClassicTlasBuildFlags buildFlags = ResolveClassicTlasBuildFlags(*m_renderHardwareInterface);
	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetRayTracingService().GetTopLevelAccelerationStructurePrebuildInfo(instanceCapacity, buildFlags);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || ResolveScratchSize(prebuildInfo, buildFlags) == 0)
	{
		Diagnostics::Fatal(
		    g_rayTracingClassicTlasBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Classic TLAS prebuild sizing produced an unusable resource layout.");
	}

	EnsureResources(prebuildInfo);
	m_tlas = TlasHandle{
	    .resource = m_accelerationStructureBuffer,
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_accelerationStructureBuffer),
	    .instanceCount = 0};
	if (!m_tlas.IsValid())
	{
		Diagnostics::Fatal(g_rayTracingClassicTlasBuilderLogger, __FILE__, __LINE__, "Classic TLAS storage has no GPU address.");
	}
}

std::uint32_t RayTracingClassicTlasBuilder::Build(
    RenderCommandContext& commandContext,
	const PreparedRenderScene& preparedScene,
	RayTracingBlasCache& blasCache,
	const RayTracingShaderTablePlan& shaderTablePlan,
	RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		Diagnostics::Fatal(
		    g_rayTracingClassicTlasBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Classic TLAS build has no render hardware interface.");
	}

	const RenderRayTracingWorkPlan& work = preparedScene.rayTracingWork;
	BuildState state;
	state.Instances.reserve(work.ClassicTlasBlasInputIndices.size());
	CollectInstances(commandContext, preparedScene, blasCache, shaderTablePlan, diagnostics, state);
	const std::uint32_t instanceCount = static_cast<std::uint32_t>(state.Instances.size());
	PrepareBuild(state);

	RecordBuild(commandContext, state, diagnostics);

	m_tlas = TlasHandle{
	    .resource = m_accelerationStructureBuffer,
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_accelerationStructureBuffer),
	    .instanceCount = static_cast<std::uint32_t>(state.Instances.size())};
	if (!m_tlas.IsValid())
	{
		Diagnostics::Fatal(g_rayTracingClassicTlasBuilderLogger, __FILE__, __LINE__, "Classic TLAS build did not publish a GPU resource.");
	}
	return instanceCount;
}

void RayTracingClassicTlasBuilder::CollectInstances(
    RenderCommandContext& commandContext,
	const PreparedRenderScene& preparedScene,
	RayTracingBlasCache& blasCache,
	const RayTracingShaderTablePlan& shaderTablePlan,
	RayTracingPerformanceDiagnostics* diagnostics,
    BuildState& state) noexcept
{
	const RenderRayTracingWorkPlan& work = preparedScene.rayTracingWork;
	for (const std::uint32_t blasInputIndex : work.ClassicTlasBlasInputIndices)
	{
		if (blasInputIndex >= work.BlasInputs.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingClassicTlasBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "Classic TLAS work references a BLAS input outside the prepared work plan.");
		}
		const RenderRayTracingBlasInput& input = work.BlasInputs[blasInputIndex];
		if (input.PrimitiveIndex >= preparedScene.primitives.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingClassicTlasBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "Classic TLAS work references a mesh instance outside the render scene.");
		}
		const MeshDraw& draw = preparedScene.primitives[input.PrimitiveIndex].Draw;
		if (!draw.Geometry.Mesh)
		{
			Diagnostics::Fatal(
			    g_rayTracingClassicTlasBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "Classic TLAS work references a mesh instance with no GPU mesh handle.");
		}

		const RayTracingBlasCache::BlasHandle blas =
		    blasCache.EnsureBlas(commandContext, preparedScene, draw, input.GpuSceneSlot, diagnostics);

		commandContext.TrackResource(blas.resource);
		if (blas.builtThisFrame)
		{
			state.BuiltBlasResources.insert(blas.resource.Value);
		}
		std::uint32_t instanceContribution = 0u;
		if (!shaderTablePlan.ResolveInstanceContribution(input.GpuSceneSlot, instanceContribution))
		{
			Diagnostics::Fatal(
			    g_rayTracingClassicTlasBuilderLogger,
			    __FILE__,
			    __LINE__,
			    "Classic TLAS instance has no authoritative scene shader-table contribution.");
		}
		state.Instances.push_back(
		    RhiRayTracingInstanceDesc{
		        .Transform = RhiRayTracingTransformPacking::PackCanonicalObjectToWorld(draw.Transform.WorldMatrix),
		        .InstanceID = input.GpuSceneSlot,
		        .InstanceMask = 0xFFu,
		        .InstanceContributionToHitGroupIndex = instanceContribution,
		        .Flags = ResolveInstanceFlags(preparedScene, draw),
		        .AccelerationStructure = blas.gpuAddress});
	}
}

void RayTracingClassicTlasBuilder::PrepareBuild(BuildState& state) noexcept
{
	state.RequestedFlags = ResolveClassicTlasBuildFlags(*m_renderHardwareInterface);
	state.PrebuildInfo = m_renderHardwareInterface->GetRayTracingService().GetTopLevelAccelerationStructurePrebuildInfo(
	    static_cast<std::uint32_t>(state.Instances.size()),
	    state.RequestedFlags);
	const std::uint64_t scratchSizeInBytes = ResolveScratchSize(state.PrebuildInfo, state.RequestedFlags);
	if (state.PrebuildInfo.ResultDataMaxSizeInBytes == 0 || scratchSizeInBytes == 0)
	{
		Diagnostics::Fatal(
		    g_rayTracingClassicTlasBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Classic TLAS build sizing produced an unusable resource layout.");
	}
	EnsureResources(state.PrebuildInfo);

	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	if (m_instanceBuffer)
	{
		resourceService.ReleaseOwnedResource(m_instanceBuffer);
		m_instanceBuffer = {};
	}
	m_instanceBuffer = m_renderHardwareInterface->GetRayTracingService().CreateRayTracingInstanceBuffer(
	    state.Instances.empty() ? nullptr : state.Instances.data(),
	    static_cast<std::uint32_t>(state.Instances.size()),
	    L"RayTracingTlasInstances");
	if (!m_instanceBuffer)
	{
		Diagnostics::Fatal(g_rayTracingClassicTlasBuilderLogger, __FILE__, __LINE__, "Classic TLAS instance-buffer allocation failed.");
	}

	const bool canRefit = !state.Instances.empty() && HasFlag(state.RequestedFlags, ERhiClassicTlasBuildFlags::AllowUpdate)
	    && m_resourcesAllowUpdate && m_tlas.IsValid() && m_tlas.instanceCount == state.Instances.size()
	    && state.PrebuildInfo.UpdateScratchDataSizeInBytes > 0;
	state.Mode = canRefit
	    ? ERhiClassicTlasBuildMode::Update
	    : (HasFlag(state.RequestedFlags, ERhiClassicTlasBuildFlags::AllowUpdate) ? ERhiClassicTlasBuildMode::BuildAllowUpdate
	                                                                             : ERhiClassicTlasBuildMode::Build);
	state.EventName = canRefit ? "Classic TLAS Refit" : "Classic TLAS Build";
}

void RayTracingClassicTlasBuilder::RecordBuild(
    RenderCommandContext& commandContext,
    const BuildState& state,
    RayTracingPerformanceDiagnostics* diagnostics) const noexcept
{
	for (void* resourceValue : state.BuiltBlasResources)
	{
		commandContext.UnorderedAccessBarrier(RhiResourceHandle{resourceValue});
	}
	TrackBuildResources(commandContext);

	auto tlasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuScope(state.EventName) : ScopedGpuScope{};
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	commandContext.BuildTopLevelAccelerationStructure(
	    resourceService.GetResourceGpuVirtualAddress(m_instanceBuffer),
	    static_cast<std::uint32_t>(state.Instances.size()),
	    resourceService.GetResourceGpuVirtualAddress(m_scratchBuffer),
	    resourceService.GetResourceGpuVirtualAddress(m_accelerationStructureBuffer),
	    state.Mode);
}

void RayTracingClassicTlasBuilder::TrackBuildResources(RenderCommandContext& commandContext) const noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

	commandContext.TrackResource(resources.GetResourceHandle(m_instanceBuffer));
	commandContext.TrackResource(resources.GetResourceHandle(m_scratchBuffer));
	commandContext.TrackResource(resources.GetResourceHandle(m_accelerationStructureBuffer));
}

void RayTracingClassicTlasBuilder::Clear() noexcept
{
	ReleaseResources();
	m_tlas = {};
}

void RayTracingClassicTlasBuilder::ReleaseResources() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		m_instanceBuffer = {};
		m_scratchBuffer = {};
		m_accelerationStructureBuffer = {};
		m_scratchBufferSizeInBytes = 0;
		m_accelerationStructureSizeInBytes = 0;
		m_resourcesAllowUpdate = false;
		return;
	}

	if (m_instanceBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_instanceBuffer);
	}
	if (m_scratchBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_scratchBuffer);
	}
	if (m_accelerationStructureBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_accelerationStructureBuffer);
	}

	m_instanceBuffer = {};
	m_scratchBuffer = {};
	m_accelerationStructureBuffer = {};
	m_scratchBufferSizeInBytes = 0;
	m_accelerationStructureSizeInBytes = 0;
	m_resourcesAllowUpdate = false;
}

void RayTracingClassicTlasBuilder::EnsureResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		Diagnostics::Fatal(
		    g_rayTracingClassicTlasBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Classic TLAS resource allocation has no render hardware interface.");
	}

	const ERhiClassicTlasBuildFlags requestedBuildFlags = ResolveClassicTlasBuildFlags(*m_renderHardwareInterface);
	const bool requestedAllowUpdate = HasFlag(requestedBuildFlags, ERhiClassicTlasBuildFlags::AllowUpdate);
	const std::uint64_t scratchSizeInBytes = ResolveScratchSize(prebuildInfo, requestedBuildFlags);
	if (m_resourcesAllowUpdate != requestedAllowUpdate)
	{
		if (m_scratchBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_scratchBuffer);
			m_scratchBuffer = {};
			m_scratchBufferSizeInBytes = 0;
		}
		if (m_accelerationStructureBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_accelerationStructureBuffer);
			m_accelerationStructureBuffer = {};
			m_accelerationStructureSizeInBytes = 0;
		}
		m_tlas = {};
		m_resourcesAllowUpdate = requestedAllowUpdate;
	}

	if (m_scratchBuffer && m_scratchBufferSizeInBytes < scratchSizeInBytes)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_scratchBuffer);
		m_scratchBuffer = {};
		m_scratchBufferSizeInBytes = 0;
	}
	if (m_accelerationStructureBuffer && m_accelerationStructureSizeInBytes < prebuildInfo.ResultDataMaxSizeInBytes)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_accelerationStructureBuffer);
		m_accelerationStructureBuffer = {};
		m_accelerationStructureSizeInBytes = 0;
	}

	if (!m_scratchBuffer)
	{
		const std::uint64_t alignedScratchSize = AlignRayTracingBufferSize(
		    scratchSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.ScratchBufferByteAlignment);
		m_scratchBuffer =
		    m_renderHardwareInterface->GetRayTracingService().CreateRayTracingScratchBuffer(alignedScratchSize, L"RayTracingTlasScratch");
		m_scratchBufferSizeInBytes = alignedScratchSize;
	}
	if (!m_accelerationStructureBuffer)
	{
		const std::uint64_t alignedAccelerationStructureSize = AlignRayTracingBufferSize(
		    prebuildInfo.ResultDataMaxSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.AccelerationStructureByteAlignment);
		m_accelerationStructureBuffer = m_renderHardwareInterface->GetRayTracingService().CreateRayTracingAccelerationStructureBuffer(
		    alignedAccelerationStructureSize,
		    ERhiRayTracingAccelerationStructureType::TopLevel,
		    L"RayTracingTlas");
		m_accelerationStructureSizeInBytes = alignedAccelerationStructureSize;
	}

	if (!m_scratchBuffer || !m_accelerationStructureBuffer)
	{
		Diagnostics::Fatal(
		    g_rayTracingClassicTlasBuilderLogger,
		    __FILE__,
		    __LINE__,
		    "Classic TLAS scratch or acceleration-structure allocation failed.");
	}
}
