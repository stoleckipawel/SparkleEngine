#include "PCH.h"

#include "SceneData/RayTracing/RayTracingSceneManager.h"

#include "GPU/CommandContext.h"
#include "GPU/GPUMesh.h"
#include "Renderer/Public/SceneData/MeshDraw.h"
#include "SceneData/RenderSceneData.h"

static const auto g_rayTracingSceneLogger = Logging::GetOrCreateLogger("Renderer.RayTracingScene");

RayTracingSceneManager::~RayTracingSceneManager() noexcept
{
	Reset();
}

void RayTracingSceneManager::Update(
    RenderHardwareInterface& renderHardwareInterface,
    CommandContext& commandContext,
    const RenderSceneData& sceneData)
{
	m_renderHardwareInterface = &renderHardwareInterface;
	m_diagnostics = {};

	const RhiRayTracingCapabilities capabilities = renderHardwareInterface.GetRayTracingCapabilities();
	if (!capabilities.SupportsInlineRayQuery)
	{
		Reset();
		return;
	}

	for (const MeshDraw& draw : sceneData.meshDraws)
	{
		if (draw.gpuMesh == nullptr || !draw.gpuMesh->IsValid())
		{
			continue;
		}

		if (EnsureBottomLevel(renderHardwareInterface, commandContext, *draw.gpuMesh))
		{
			++m_diagnostics.rebuiltBottomLevelCount;
		}
	}

	m_diagnostics.bottomLevelCount = static_cast<std::uint32_t>(m_bottomLevelRecords.size());
	RebuildTopLevel(renderHardwareInterface, commandContext, sceneData);
}

void RayTracingSceneManager::Reset() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		m_bottomLevelRecords.clear();
		m_topLevelResult = {};
		m_topLevelScratch = {};
		m_instanceBuffer = {};
		m_topLevelGpuAddress = 0;
		m_topLevelResultSizeInBytes = 0;
		m_topLevelScratchSizeInBytes = 0;
		m_diagnostics = {};
		return;
	}

	for (auto& entry : m_bottomLevelRecords)
	{
		ReleaseBottomLevelRecord(*m_renderHardwareInterface, entry.second);
	}
	m_bottomLevelRecords.clear();
	ReleaseTopLevelResources(*m_renderHardwareInterface);
	m_diagnostics = {};
}

bool RayTracingSceneManager::EnsureBottomLevel(
    RenderHardwareInterface& renderHardwareInterface,
    CommandContext& commandContext,
    const GPUMesh& gpuMesh)
{
	const RhiRayTracingGeometryDesc geometry = gpuMesh.GetRayTracingGeometry();
	BottomLevelRecord& record = m_bottomLevelRecords[&gpuMesh];
	const bool needsRebuild = record.gpuAddress == 0 || !GeometryMatches(record.geometry, geometry);
	if (!needsRebuild)
	{
		return false;
	}

	ReleaseBottomLevelRecord(renderHardwareInterface, record);
	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    renderHardwareInterface.GetBottomLevelAccelerationStructurePrebuildInfo(geometry);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
	{
		SPDLOG_LOGGER_WARN(g_rayTracingSceneLogger, "[RayTracingScene] Failed to query BLAS prebuild info");
		return false;
	}

	record.result = renderHardwareInterface.CreateRayTracingAccelerationStructureBuffer(prebuildInfo.ResultDataMaxSizeInBytes, L"BLAS_Result");
	record.scratch = renderHardwareInterface.CreateRayTracingScratchBuffer(prebuildInfo.ScratchDataSizeInBytes, L"BLAS_Scratch");
	if (!record.result || !record.scratch)
	{
		ReleaseBottomLevelRecord(renderHardwareInterface, record);
		SPDLOG_LOGGER_WARN(g_rayTracingSceneLogger, "[RayTracingScene] Failed to allocate BLAS resources");
		return false;
	}

	record.gpuAddress = renderHardwareInterface.GetResourceGpuVirtualAddress(record.result);
	record.geometry = geometry;
	record.resultSizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes;
	commandContext.BuildBottomLevelAccelerationStructure(
	    geometry,
	    renderHardwareInterface.GetResourceGpuVirtualAddress(record.scratch),
	    record.gpuAddress);
	commandContext.UnorderedAccessBarrier(renderHardwareInterface.GetNativeResource(record.result));
	return true;
}

void RayTracingSceneManager::RebuildTopLevel(
    RenderHardwareInterface& renderHardwareInterface,
    CommandContext& commandContext,
    const RenderSceneData& sceneData)
{
	std::vector<RhiRayTracingInstanceDesc> instances;
	instances.reserve(sceneData.meshDraws.size());
	for (const MeshDraw& draw : sceneData.meshDraws)
	{
		if (draw.gpuMesh == nullptr)
		{
			continue;
		}

		const auto found = m_bottomLevelRecords.find(draw.gpuMesh);
		if (found == m_bottomLevelRecords.end() || found->second.gpuAddress == 0)
		{
			continue;
		}

		instances.push_back(BuildInstanceDesc(draw, found->second.gpuAddress, static_cast<std::uint32_t>(instances.size())));
	}

	m_diagnostics.topLevelInstanceCount = static_cast<std::uint32_t>(instances.size());
	if (instances.empty())
	{
		ReleaseTopLevelResources(renderHardwareInterface);
		return;
	}

	if (m_instanceBuffer)
	{
		renderHardwareInterface.ReleaseOwnedResource(m_instanceBuffer);
		m_instanceBuffer = {};
	}
	m_instanceBuffer = renderHardwareInterface.CreateRayTracingInstanceBuffer(instances.data(), static_cast<std::uint32_t>(instances.size()), L"TLAS_Instances");
	if (!m_instanceBuffer)
	{
		ReleaseTopLevelResources(renderHardwareInterface);
		SPDLOG_LOGGER_WARN(g_rayTracingSceneLogger, "[RayTracingScene] Failed to allocate TLAS instance buffer");
		return;
	}

	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    renderHardwareInterface.GetTopLevelAccelerationStructurePrebuildInfo(static_cast<std::uint32_t>(instances.size()));
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
	{
		ReleaseTopLevelResources(renderHardwareInterface);
		SPDLOG_LOGGER_WARN(g_rayTracingSceneLogger, "[RayTracingScene] Failed to query TLAS prebuild info");
		return;
	}

	if (!m_topLevelResult || prebuildInfo.ResultDataMaxSizeInBytes > m_topLevelResultSizeInBytes)
	{
		if (m_topLevelResult)
		{
			renderHardwareInterface.ReleaseOwnedResource(m_topLevelResult);
		}
		m_topLevelResult = renderHardwareInterface.CreateRayTracingAccelerationStructureBuffer(prebuildInfo.ResultDataMaxSizeInBytes, L"TLAS_Result");
		m_topLevelResultSizeInBytes = prebuildInfo.ResultDataMaxSizeInBytes;
		m_topLevelGpuAddress = renderHardwareInterface.GetResourceGpuVirtualAddress(m_topLevelResult);
	}

	if (!m_topLevelScratch || prebuildInfo.ScratchDataSizeInBytes > m_topLevelScratchSizeInBytes)
	{
		if (m_topLevelScratch)
		{
			renderHardwareInterface.ReleaseOwnedResource(m_topLevelScratch);
		}
		m_topLevelScratch = renderHardwareInterface.CreateRayTracingScratchBuffer(prebuildInfo.ScratchDataSizeInBytes, L"TLAS_Scratch");
		m_topLevelScratchSizeInBytes = prebuildInfo.ScratchDataSizeInBytes;
	}

	if (!m_topLevelResult || !m_topLevelScratch)
	{
		ReleaseTopLevelResources(renderHardwareInterface);
		SPDLOG_LOGGER_WARN(g_rayTracingSceneLogger, "[RayTracingScene] Failed to allocate TLAS resources");
		return;
	}

	commandContext.BuildTopLevelAccelerationStructure(
	    renderHardwareInterface.GetResourceGpuVirtualAddress(m_instanceBuffer),
	    static_cast<std::uint32_t>(instances.size()),
	    renderHardwareInterface.GetResourceGpuVirtualAddress(m_topLevelScratch),
	    m_topLevelGpuAddress);
	commandContext.UnorderedAccessBarrier(renderHardwareInterface.GetNativeResource(m_topLevelResult));
	m_diagnostics.rebuiltTopLevel = true;
}

void RayTracingSceneManager::ReleaseBottomLevelRecord(
    RenderHardwareInterface& renderHardwareInterface,
    BottomLevelRecord& record) noexcept
{
	if (record.result)
	{
		renderHardwareInterface.ReleaseOwnedResource(record.result);
	}
	if (record.scratch)
	{
		renderHardwareInterface.ReleaseOwnedResource(record.scratch);
	}
	record = {};
}

void RayTracingSceneManager::ReleaseTopLevelResources(RenderHardwareInterface& renderHardwareInterface) noexcept
{
	if (m_topLevelResult)
	{
		renderHardwareInterface.ReleaseOwnedResource(m_topLevelResult);
	}
	if (m_topLevelScratch)
	{
		renderHardwareInterface.ReleaseOwnedResource(m_topLevelScratch);
	}
	if (m_instanceBuffer)
	{
		renderHardwareInterface.ReleaseOwnedResource(m_instanceBuffer);
	}
	m_topLevelResult = {};
	m_topLevelScratch = {};
	m_instanceBuffer = {};
	m_topLevelGpuAddress = 0;
	m_topLevelResultSizeInBytes = 0;
	m_topLevelScratchSizeInBytes = 0;
}

RhiRayTracingInstanceDesc RayTracingSceneManager::BuildInstanceDesc(
    const MeshDraw& draw,
    RhiGpuVirtualAddress bottomLevelGpuAddress,
    std::uint32_t instanceId) noexcept
{
	const DirectX::XMFLOAT4X4& world = draw.worldMatrix;
	RhiRayTracingInstanceDesc instance{};
	instance.Transform = {
	    world._11,
	    world._21,
	    world._31,
	    world._41,
	    world._12,
	    world._22,
	    world._32,
	    world._42,
	    world._13,
	    world._23,
	    world._33,
	    world._43};
	instance.InstanceID = instanceId;
	instance.InstanceMask = 0xFF;
	instance.InstanceContributionToHitGroupIndex = 0;
	instance.AccelerationStructure = bottomLevelGpuAddress;
	return instance;
}

bool RayTracingSceneManager::GeometryMatches(const RhiRayTracingGeometryDesc& lhs, const RhiRayTracingGeometryDesc& rhs) noexcept
{
	return lhs.VertexBuffer == rhs.VertexBuffer && lhs.VertexStrideInBytes == rhs.VertexStrideInBytes && lhs.VertexCount == rhs.VertexCount &&
	       lhs.IndexBuffer == rhs.IndexBuffer && lhs.IndexCount == rhs.IndexCount && lhs.IndexFormat == rhs.IndexFormat && lhs.Opaque == rhs.Opaque;
}