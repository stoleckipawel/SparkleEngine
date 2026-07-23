#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingClassicTlasBuilder.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Math/MathUtils.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <unordered_set>

static const auto g_rayTracingClassicTlasBuilderLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

class RayTracingClassicTlasBuilderOperations final
{
  public:
	static std::uint64_t AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept
	{
		return alignment > 0 ? MathUtils::AlignUp(sizeInBytes, alignment) : sizeInBytes;
	}

	static bool SupportsClassicTlasRefit(RenderHardwareInterface& renderHardwareInterface) noexcept
	{
		return renderHardwareInterface.GetCapabilities().RayTracing.Groups.ClassicTlas.SupportsClassicTlasUpdate;
	}

	static ERhiClassicTlasBuildFlags ResolveClassicTlasBuildFlags(RenderHardwareInterface& renderHardwareInterface) noexcept
	{
		return CVarRayTracingClassicTlasRefit.Get() && SupportsClassicTlasRefit(renderHardwareInterface)
		           ? ERhiClassicTlasBuildFlags::AllowUpdate
		           : ERhiClassicTlasBuildFlags::None;
	}

	static std::uint64_t ResolveRequiredScratchSize(
	    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
	    ERhiClassicTlasBuildFlags buildFlags) noexcept
	{
		if (!HasFlag(buildFlags, ERhiClassicTlasBuildFlags::AllowUpdate))
		{
			return prebuildInfo.ScratchDataSizeInBytes;
		}
		return (std::max) (prebuildInfo.ScratchDataSizeInBytes, prebuildInfo.UpdateScratchDataSizeInBytes);
	}

	static RhiRayTracingInstanceFlags ResolveInstanceFlags(const RenderSceneData& sceneData, const MeshDraw& draw) noexcept
	{
		RhiRayTracingInstanceFlags flags = RhiRayTracingInstanceFlags::None;
		if (draw.Material.Slot >= sceneData.materials.size())
		{
			return flags;
		}
		const MaterialData& material = sceneData.materials[draw.Material.Slot];
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
};

RayTracingClassicTlasBuilder::RayTracingClassicTlasBuilder(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface)
{
}

RayTracingClassicTlasBuilder::~RayTracingClassicTlasBuilder() noexcept
{
	Clear();
}

bool RayTracingClassicTlasBuilder::Prepare(std::uint32_t instanceCapacity) noexcept
{
	if (m_renderHardwareInterface == nullptr || instanceCapacity == 0)
	{
		m_tlas = {};
		return false;
	}

	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetRayTracingService().GetTopLevelAccelerationStructurePrebuildInfo(
	        instanceCapacity,
	        RayTracingClassicTlasBuilderOperations::ResolveClassicTlasBuildFlags(*m_renderHardwareInterface));
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 ||
	    RayTracingClassicTlasBuilderOperations::ResolveRequiredScratchSize(prebuildInfo, RayTracingClassicTlasBuilderOperations::ResolveClassicTlasBuildFlags(*m_renderHardwareInterface)) == 0)
	{
		m_tlas = {};
		return false;
	}

	if (!EnsureResources(prebuildInfo))
	{
		m_tlas = {};
		return false;
	}

	m_tlas = TlasHandle{
	    .resource = m_accelerationStructureBuffer,
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_accelerationStructureBuffer),
	    .instanceCount = 0};
	return m_tlas.resource && m_tlas.gpuAddress != 0;
}

RayTracingClassicTlasBuilder::BuildStats RayTracingClassicTlasBuilder::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlan* partitionPlan,
    RayTracingBlasCache& blasCache,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	BuildStats stats{};
	if (m_renderHardwareInterface == nullptr)
	{
		return stats;
	}

	std::unordered_set<void*> builtBlasResources;
	std::vector<RhiRayTracingInstanceDesc> instances;
	stats.Candidates.InstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	(void) partitionPlan;
	instances.reserve(sceneData.meshInstances.size());
	{
		for (std::uint32_t index = 0; index < static_cast<std::uint32_t>(sceneData.meshInstances.size()); ++index)
		{
			const MeshDraw& draw = sceneData.meshInstances[index];
			if (draw.Geometry.GpuMesh == nullptr || !draw.Geometry.GpuMesh->IsValid())
			{
				++stats.Candidates.MissingGpuMeshCount;
				continue;
			}

			const RayTracingBlasCache::BlasHandle blas = blasCache.EnsureBlas(cmd, sceneData, draw, index, diagnostics);
			if (!blas.IsValid())
			{
				++stats.Candidates.RejectedBlasCount;
				continue;
			}
			if (blas.builtThisFrame)
			{
				builtBlasResources.insert(blas.resource.Value);
			}

			instances.push_back(
			    RhiRayTracingInstanceDesc{
			        .Transform = BuildInstanceTransform(draw.Transform.WorldMatrix),
			        .InstanceID = index,
			        .InstanceMask = 0xFFu,
			        .InstanceContributionToHitGroupIndex = 0u,
			        .Flags = RayTracingClassicTlasBuilderOperations::ResolveInstanceFlags(sceneData, draw),
			        .AccelerationStructure = blas.gpuAddress});
		}
	}

	stats.Build.InstanceCount = static_cast<std::uint32_t>(instances.size());
	if (instances.empty())
	{
		m_tlas = {};
		if (m_instanceBuffer)
		{
			m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_instanceBuffer);
			m_instanceBuffer = {};
		}
		return stats;
	}

	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetRayTracingService().GetTopLevelAccelerationStructurePrebuildInfo(
	        stats.Build.InstanceCount,
	        RayTracingClassicTlasBuilderOperations::ResolveClassicTlasBuildFlags(*m_renderHardwareInterface));
	const ERhiClassicTlasBuildFlags requestedBuildFlags = RayTracingClassicTlasBuilderOperations::ResolveClassicTlasBuildFlags(*m_renderHardwareInterface);
	const std::uint64_t requiredScratchSize = RayTracingClassicTlasBuilderOperations::ResolveRequiredScratchSize(prebuildInfo, requestedBuildFlags);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || requiredScratchSize == 0)
	{
		m_tlas = {};
		SPDLOG_LOGGER_WARN(
		    g_rayTracingClassicTlasBuilderLogger,
		    "RayTracingClassicTlasBuilder: skipping TLAS build because prebuild info was invalid (instanceCount={}).",
		    stats.Build.InstanceCount);
		return stats;
	}

	if (!EnsureResources(prebuildInfo))
	{
		m_tlas = {};
		return stats;
	}

	if (m_instanceBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(m_instanceBuffer);
		m_instanceBuffer = {};
	}

	m_instanceBuffer = m_renderHardwareInterface->GetRayTracingService().CreateRayTracingInstanceBuffer(
	    instances.data(),
	    stats.Build.InstanceCount,
	    L"RayTracingTlasInstances");
	if (!m_instanceBuffer)
	{
		m_tlas = {};
		SPDLOG_LOGGER_WARN(
		    g_rayTracingClassicTlasBuilderLogger,
		    "RayTracingClassicTlasBuilder: failed to upload {} TLAS instances.",
		    stats.Build.InstanceCount);
		return stats;
	}

	for (void* resourceValue : builtBlasResources)
	{
		cmd.UnorderedAccessBarrier(RhiResourceHandle{resourceValue});
	}

	const bool canRefit = HasFlag(requestedBuildFlags, ERhiClassicTlasBuildFlags::AllowUpdate) && m_resourcesAllowUpdate &&
	                      m_tlas.IsValid() && m_tlas.instanceCount == stats.Build.InstanceCount &&
	                      prebuildInfo.UpdateScratchDataSizeInBytes > 0;
	const ERhiClassicTlasBuildMode buildMode =
	    canRefit ? ERhiClassicTlasBuildMode::Update
	             : (HasFlag(requestedBuildFlags, ERhiClassicTlasBuildFlags::AllowUpdate)
	                    ? ERhiClassicTlasBuildMode::BuildAllowUpdate
	                    : ERhiClassicTlasBuildMode::Build);
	const char* const tlasEventName = canRefit ? "Classic TLAS Refit" : "Classic TLAS Build";
	{
		auto tlasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuScope(tlasEventName) : ScopedGpuScope{};
		cmd.BuildTopLevelAccelerationStructure(
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_instanceBuffer),
		    stats.Build.InstanceCount,
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_scratchBuffer),
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_accelerationStructureBuffer),
		    buildMode);
	}

	m_tlas = TlasHandle{
	    .resource = m_accelerationStructureBuffer,
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_accelerationStructureBuffer),
	    .instanceCount = stats.Build.InstanceCount};
	stats.Build.Built = m_tlas.IsValid();
	return stats;
}

void RayTracingClassicTlasBuilder::Clear() noexcept
{
	ReleaseResources();
	m_tlas = {};
}

std::array<float, 12> RayTracingClassicTlasBuilder::BuildInstanceTransform(const DirectX::XMFLOAT4X4& worldMatrix) noexcept
{
	return {
	    worldMatrix._11,
	    worldMatrix._12,
	    worldMatrix._13,
	    worldMatrix._14,
	    worldMatrix._21,
	    worldMatrix._22,
	    worldMatrix._23,
	    worldMatrix._24,
	    worldMatrix._31,
	    worldMatrix._32,
	    worldMatrix._33,
	    worldMatrix._34};
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

bool RayTracingClassicTlasBuilder::EnsureResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return false;
	}

	const ERhiClassicTlasBuildFlags requestedBuildFlags = RayTracingClassicTlasBuilderOperations::ResolveClassicTlasBuildFlags(*m_renderHardwareInterface);
	const bool requestedAllowUpdate = HasFlag(requestedBuildFlags, ERhiClassicTlasBuildFlags::AllowUpdate);
	const std::uint64_t requiredScratchSize = RayTracingClassicTlasBuilderOperations::ResolveRequiredScratchSize(prebuildInfo, requestedBuildFlags);
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

	if (m_scratchBuffer && m_scratchBufferSizeInBytes < requiredScratchSize)
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
		const std::uint64_t alignedScratchSize = RayTracingClassicTlasBuilderOperations::AlignRayTracingBufferSize(
		    requiredScratchSize,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.ScratchBufferByteAlignment);
		m_scratchBuffer =
		    m_renderHardwareInterface->GetRayTracingService().CreateRayTracingScratchBuffer(alignedScratchSize, L"RayTracingTlasScratch");
		m_scratchBufferSizeInBytes = alignedScratchSize;
	}
	if (!m_accelerationStructureBuffer)
	{
		const std::uint64_t alignedAccelerationStructureSize = RayTracingClassicTlasBuilderOperations::AlignRayTracingBufferSize(
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
		SPDLOG_LOGGER_WARN(
		    g_rayTracingClassicTlasBuilderLogger,
		    "RayTracingClassicTlasBuilder: failed to allocate TLAS scratch or result buffers.");
		return false;
	}

	return true;
}
