#include "PCH.h"

#include "RayTracing/RayTracingTlasBuilder.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Math/MathUtils.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/RayTracingPerformanceDiagnostics.h"
#include "SceneData/RenderSceneData.h"

#include <unordered_set>

static const auto g_rayTracingTlasBuilderLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

namespace
{
	std::uint64_t AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept
	{
		return alignment > 0 ? MathUtils::AlignUp(sizeInBytes, alignment) : sizeInBytes;
	}
}

RayTracingTlasBuilder::RayTracingTlasBuilder(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface)
{
}

RayTracingTlasBuilder::~RayTracingTlasBuilder() noexcept
{
	Clear();
}

bool RayTracingTlasBuilder::Prepare(std::uint32_t instanceCapacity) noexcept
{
	if (m_renderHardwareInterface == nullptr || instanceCapacity == 0)
	{
		m_tlas = {};
		return false;
	}

	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetRayTracingService().GetTopLevelAccelerationStructurePrebuildInfo(instanceCapacity);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
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

RayTracingTlasBuilder::BuildStats RayTracingTlasBuilder::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingBlasCache& blasCache,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	BuildStats stats{};
	auto tlasCpuScope = diagnostics != nullptr ? diagnostics->BeginTlasCpuScope() : RayTracingPerformanceDiagnostics::CpuScope{};
	if (m_renderHardwareInterface == nullptr)
	{
		return stats;
	}

	std::unordered_set<void*> builtBlasResources;
	std::vector<RhiRayTracingInstanceDesc> instances;
	stats.candidateInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	instances.reserve(sceneData.meshInstances.size());
	{
		auto instancePrepCpuScope =
		    diagnostics != nullptr ? diagnostics->BeginTlasInstancePreparationCpuScope() : RayTracingPerformanceDiagnostics::CpuScope{};
		for (std::uint32_t index = 0; index < static_cast<std::uint32_t>(sceneData.meshInstances.size()); ++index)
		{
			const MeshDraw& draw = sceneData.meshInstances[index];
			if (draw.gpuMesh == nullptr || !draw.gpuMesh->IsValid())
			{
				++stats.missingGpuMeshCount;
				continue;
			}

			const RayTracingBlasCache::BlasHandle blas = blasCache.EnsureBlas(cmd, *draw.gpuMesh, diagnostics);
			if (!blas.IsValid())
			{
				++stats.rejectedBlasCount;
				continue;
			}
			if (blas.builtThisFrame)
			{
				builtBlasResources.insert(blas.resource.Value);
			}

			instances.push_back(
			    RhiRayTracingInstanceDesc{
			        .Transform = BuildInstanceTransform(draw.worldMatrix),
			        .InstanceID = index,
			        .InstanceMask = 0xFFu,
			        .InstanceContributionToHitGroupIndex = 0u,
			        .AccelerationStructure = blas.gpuAddress});
		}
	}

	stats.instanceCount = static_cast<std::uint32_t>(instances.size());
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
	    m_renderHardwareInterface->GetRayTracingService().GetTopLevelAccelerationStructurePrebuildInfo(stats.instanceCount);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
	{
		m_tlas = {};
		SPDLOG_LOGGER_WARN(
		    g_rayTracingTlasBuilderLogger,
		    "RayTracingTlasBuilder: skipping TLAS build because prebuild info was invalid (instanceCount={}).",
		    stats.instanceCount);
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
	    stats.instanceCount,
	    L"RayTracingTlasInstances");
	if (!m_instanceBuffer)
	{
		m_tlas = {};
		SPDLOG_LOGGER_WARN(
		    g_rayTracingTlasBuilderLogger,
		    "RayTracingTlasBuilder: failed to upload {} TLAS instances.",
		    stats.instanceCount);
		return stats;
	}

	for (void* resourceValue : builtBlasResources)
	{
		cmd.UnorderedAccessBarrier(NativeResourceHandle{resourceValue});
	}

	{
		auto tlasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuEvent("Classic TLAS Build") : ScopedGpuEvent{};
		auto tlasGpuTimer = diagnostics != nullptr ? diagnostics->BeginGpuTimer("Classic TLAS Build") : ScopedGpuTimer{};
		cmd.BuildTopLevelAccelerationStructure(
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_instanceBuffer),
		    stats.instanceCount,
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_scratchBuffer),
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_accelerationStructureBuffer));
	}

	m_tlas = TlasHandle{
	    .resource = m_accelerationStructureBuffer,
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(m_accelerationStructureBuffer),
	    .instanceCount = stats.instanceCount};
	stats.builtTlas = m_tlas.IsValid();
	return stats;
}

void RayTracingTlasBuilder::Clear() noexcept
{
	ReleaseResources();
	m_tlas = {};
}

std::array<float, 12> RayTracingTlasBuilder::BuildInstanceTransform(const DirectX::XMFLOAT4X4& worldMatrix) noexcept
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

void RayTracingTlasBuilder::ReleaseResources() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		m_instanceBuffer = {};
		m_scratchBuffer = {};
		m_accelerationStructureBuffer = {};
		m_scratchBufferSizeInBytes = 0;
		m_accelerationStructureSizeInBytes = 0;
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
}

bool RayTracingTlasBuilder::EnsureResources(const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return false;
	}

	if (m_scratchBuffer && m_scratchBufferSizeInBytes < prebuildInfo.ScratchDataSizeInBytes)
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
		    prebuildInfo.ScratchDataSizeInBytes,
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
		SPDLOG_LOGGER_WARN(g_rayTracingTlasBuilderLogger, "RayTracingTlasBuilder: failed to allocate TLAS scratch or result buffers.");
		return false;
	}

	return true;
}
