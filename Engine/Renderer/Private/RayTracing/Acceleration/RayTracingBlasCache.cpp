#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingBlasCache.h"

#include "Commands/RenderCommandContext.h"
#include "Core/Public/Math/MathUtils.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "SceneData/RenderSceneData.h"
#include "ShaderData/MeshInstanceShaderData.h"

#include <DirectXMath.h>
#include <utility>
#include <vector>

static const auto g_rayTracingBlasCacheLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

namespace
{
	bool GeometryEquals(const RhiRayTracingGeometryDesc& left, const RhiRayTracingGeometryDesc& right) noexcept
	{
		return left.VertexBuffer == right.VertexBuffer && left.VertexStrideInBytes == right.VertexStrideInBytes &&
		       left.VertexCount == right.VertexCount && left.IndexBuffer == right.IndexBuffer && left.IndexCount == right.IndexCount &&
		       left.IndexFormat == right.IndexFormat && left.Opaque == right.Opaque;
	}

	std::uint64_t AlignRayTracingBufferSize(std::uint64_t sizeInBytes, std::uint64_t alignment) noexcept
	{
		return alignment > 0 ? MathUtils::AlignUp(sizeInBytes, alignment) : sizeInBytes;
	}

	bool IsSkinnedDraw(const MeshDraw& draw) noexcept
	{
		return draw.Geometry.MeshKind == RenderMeshKind::Skeletal;
	}

	DirectX::XMFLOAT3 TransformSkinnedPosition(
	    const DirectX::XMFLOAT3& position,
	    const VertexSkinInfluence& influence,
	    std::uint32_t jointMatrixOffset,
	    const std::vector<DirectX::XMFLOAT4X4>& jointMatrices) noexcept
	{
		const DirectX::XMVECTOR sourcePosition = DirectX::XMLoadFloat3(&position);
		DirectX::XMVECTOR skinnedPosition = DirectX::XMVectorZero();
		float totalWeight = 0.0f;
		for (std::uint32_t influenceIndex = 0u; influenceIndex < 4u; ++influenceIndex)
		{
			const float weight = influence.jointWeights[influenceIndex];
			if (weight <= 0.0f)
			{
				continue;
			}

			const std::uint32_t jointMatrixIndex = jointMatrixOffset + influence.jointIndices[influenceIndex];
			if (jointMatrixIndex >= jointMatrices.size())
			{
				continue;
			}

			const DirectX::XMMATRIX skinningMatrix = DirectX::XMLoadFloat4x4(&jointMatrices[jointMatrixIndex]);
			skinnedPosition = DirectX::XMVectorAdd(
			    skinnedPosition,
			    DirectX::XMVectorScale(DirectX::XMVector3Transform(sourcePosition, skinningMatrix), weight));
			totalWeight += weight;
		}

		DirectX::XMFLOAT3 result = position;
		DirectX::XMStoreFloat3(&result, totalWeight > 0.0f ? skinnedPosition : sourcePosition);
		return result;
	}
}

RayTracingBlasCache::RayTracingBlasCache(RenderHardwareInterface& renderHardwareInterface) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface)
{
}

RayTracingBlasCache::~RayTracingBlasCache() noexcept
{
	Clear();
}

void RayTracingBlasCache::BeginFrame() noexcept
{
	m_currentFrameStats = {};
	for (auto& [mesh, entry] : m_entries)
	{
		entry.touchedThisFrame = false;
	}
	for (auto& [key, entry] : m_skinnedEntries)
	{
		entry.touchedThisFrame = false;
	}
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureBlas(
    RenderCommandContext& cmd,
    const GPUMesh& gpuMesh,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{

	if (m_renderHardwareInterface == nullptr || !gpuMesh.IsValid())
	{
		return {};
	}

	++m_currentFrameStats.referencedMeshCount;

	const RhiRayTracingGeometryDesc geometry = gpuMesh.GetRayTracingGeometry();
	auto [it, inserted] = m_entries.try_emplace(&gpuMesh);
	Entry& entry = it->second;
	entry.touchedThisFrame = true;

	const bool geometryChanged = inserted || !GeometryMatches(entry, geometry);
	if (!geometryChanged && entry.accelerationStructureBuffer)
	{
		++m_currentFrameStats.reusedBlasCount;
		return BuildHandle(entry);
	}

	return BuildBlas(cmd, geometry, entry, diagnostics);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureBlas(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    std::uint32_t renderInstanceIndex,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	if (IsSkinnedDraw(draw))
	{
		return EnsureSkinnedBlas(cmd, sceneData, draw, renderInstanceIndex, diagnostics);
	}

	if (draw.Geometry.GpuMesh == nullptr)
	{
		return {};
	}
	return EnsureBlas(cmd, *draw.Geometry.GpuMesh, diagnostics);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::BuildBlas(
    RenderCommandContext& cmd,
    const RhiRayTracingGeometryDesc& geometry,
    Entry& entry,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	const RhiRayTracingAccelerationStructurePrebuildInfo prebuildInfo =
	    m_renderHardwareInterface->GetRayTracingService().GetBottomLevelAccelerationStructurePrebuildInfo(geometry);
	if (prebuildInfo.ResultDataMaxSizeInBytes == 0 || prebuildInfo.ScratchDataSizeInBytes == 0)
	{
		SPDLOG_LOGGER_WARN(
		    g_rayTracingBlasCacheLogger,
		    "RayTracingBlasCache: skipping BLAS build because prebuild info was invalid (vertexCount={}, indexCount={}).",
		    geometry.VertexCount,
		    geometry.IndexCount);
		return {};
	}

	if (!EnsureEntryResources(geometry, prebuildInfo, entry))
	{
		return {};
	}

	entry.geometry = geometry;
	{
		auto blasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuScope("BLAS Build") : ScopedGpuScope{};
		cmd.BuildBottomLevelAccelerationStructure(
		    geometry,
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(entry.scratchBuffer),
		    m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(entry.accelerationStructureBuffer));
	}
	++m_currentFrameStats.builtBlasCount;
	BlasHandle handle = BuildHandle(entry);
	handle.builtThisFrame = true;
	return handle;
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::EnsureSkinnedBlas(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    std::uint32_t renderInstanceIndex,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	if (m_renderHardwareInterface == nullptr || draw.Geometry.GpuMesh == nullptr || !draw.Geometry.GpuMesh->IsValid())
	{
		return {};
	}

	++m_currentFrameStats.referencedMeshCount;

	SkinnedEntryKey key{
	    .Mesh = draw.Geometry.GpuMesh,
	    .RenderInstanceIndex = renderInstanceIndex};
	auto [it, inserted] = m_skinnedEntries.try_emplace(key);
	(void)inserted;
	Entry& entry = it->second;
	entry.touchedThisFrame = true;

	RhiRayTracingGeometryDesc geometry{};
	if (!BuildSkinnedGeometry(sceneData, draw, entry, geometry))
	{
		ReleaseEntryResources(entry);
		return {};
	}

	return BuildBlas(cmd, geometry, entry, diagnostics);
}

RayTracingBlasCache::BuildStats RayTracingBlasCache::EndFrame() noexcept
{
	for (auto it = m_entries.begin(); it != m_entries.end();)
	{
		if (it->second.touchedThisFrame)
		{
			++it;
			continue;
		}

		ReleaseEntryResources(it->second);
		it = m_entries.erase(it);
	}
	for (auto it = m_skinnedEntries.begin(); it != m_skinnedEntries.end();)
	{
		if (it->second.touchedThisFrame)
		{
			++it;
			continue;
		}

		ReleaseEntryResources(it->second);
		it = m_skinnedEntries.erase(it);
	}

	return m_currentFrameStats;
}

void RayTracingBlasCache::Clear() noexcept
{
	for (auto& [mesh, entry] : m_entries)
	{
		ReleaseEntryResources(entry);
	}

	m_entries.clear();
	m_skinnedEntries.clear();
	m_currentFrameStats = {};
}

void RayTracingBlasCache::ReleaseEntryResources(Entry& entry) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		entry = {};
		return;
	}

	if (entry.scratchBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.scratchBuffer);
	}
	if (entry.dynamicVertexBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.dynamicVertexBuffer);
	}
	if (entry.accelerationStructureBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.accelerationStructureBuffer);
	}

	entry = {};
}

bool RayTracingBlasCache::BuildSkinnedGeometry(
    const RenderSceneData& sceneData,
    const MeshDraw& draw,
    Entry& entry,
    RhiRayTracingGeometryDesc& outGeometry) noexcept
{
	if (m_renderHardwareInterface == nullptr || draw.Geometry.GpuMesh == nullptr ||
	    draw.Skinning.JointMatrixOffset == kInvalidMeshInstanceJointMatrixOffset)
	{
		return false;
	}

	const GPUMesh& gpuMesh = *draw.Geometry.GpuMesh;
	if (!gpuMesh.HasRayTracingHitData() || !gpuMesh.HasSkinInfluences() ||
	    gpuMesh.GetRayTracingHitVertices().size() != gpuMesh.GetSkinInfluences().size() ||
	    sceneData.jointMatrices.empty())
	{
		return false;
	}

	std::vector<DirectX::XMFLOAT3> skinnedPositions;
	skinnedPositions.reserve(gpuMesh.GetRayTracingHitVertices().size());
	const std::span<const RayTracingHitVertex> vertices = gpuMesh.GetRayTracingHitVertices();
	const std::span<const VertexSkinInfluence> skinInfluences = gpuMesh.GetSkinInfluences();
	for (std::size_t vertexIndex = 0; vertexIndex < vertices.size(); ++vertexIndex)
	{
		for (std::uint32_t influenceIndex = 0u; influenceIndex < 4u; ++influenceIndex)
		{
			if (skinInfluences[vertexIndex].jointWeights[influenceIndex] <= 0.0f)
			{
				continue;
			}
			const std::uint32_t jointMatrixIndex =
			    draw.Skinning.JointMatrixOffset + skinInfluences[vertexIndex].jointIndices[influenceIndex];
			if (jointMatrixIndex >= sceneData.jointMatrices.size())
			{
				return false;
			}
		}
		skinnedPositions.push_back(
		    TransformSkinnedPosition(
		        vertices[vertexIndex].Position,
		        skinInfluences[vertexIndex],
		        draw.Skinning.JointMatrixOffset,
		        sceneData.jointMatrices));
	}

	if (skinnedPositions.empty())
	{
		return false;
	}

	if (entry.dynamicVertexBuffer)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.dynamicVertexBuffer);
		entry.dynamicVertexBuffer = {};
		entry.dynamicVertexBufferView = {};
	}

	if (!m_renderHardwareInterface->GetResourceService().CreateVertexBuffer(
	        skinnedPositions.data(),
	        skinnedPositions.size() * sizeof(DirectX::XMFLOAT3),
	        static_cast<std::uint32_t>(sizeof(DirectX::XMFLOAT3)),
	        L"RayTracingSkinnedBlasVertices",
	        entry.dynamicVertexBuffer,
	        entry.dynamicVertexBufferView) ||
	    !entry.dynamicVertexBuffer)
	{
		return false;
	}

	const RhiIndexBufferView indexBufferView = gpuMesh.GetIndexBufferView();
	outGeometry = RhiRayTracingGeometryDesc{
	    .VertexBuffer = entry.dynamicVertexBufferView.BufferLocation,
	    .VertexStrideInBytes = entry.dynamicVertexBufferView.StrideInBytes,
	    .VertexCount = static_cast<std::uint32_t>(skinnedPositions.size()),
	    .IndexBuffer = indexBufferView.BufferLocation,
	    .IndexCount = gpuMesh.GetIndexCount(),
	    .IndexFormat = indexBufferView.Format,
	    .Opaque = true};
	return true;
}

bool RayTracingBlasCache::EnsureEntryResources(
    const RhiRayTracingGeometryDesc& geometry,
    const RhiRayTracingAccelerationStructurePrebuildInfo& prebuildInfo,
    Entry& entry) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return false;
	}

	if (entry.scratchBuffer && entry.scratchBufferSizeInBytes < prebuildInfo.ScratchDataSizeInBytes)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.scratchBuffer);
		entry.scratchBuffer = {};
		entry.scratchBufferSizeInBytes = 0;
	}
	if (entry.accelerationStructureBuffer && entry.accelerationStructureSizeInBytes < prebuildInfo.ResultDataMaxSizeInBytes)
	{
		m_renderHardwareInterface->GetResourceService().ReleaseOwnedResource(entry.accelerationStructureBuffer);
		entry.accelerationStructureBuffer = {};
		entry.accelerationStructureSizeInBytes = 0;
	}

	if (!entry.scratchBuffer)
	{
		const std::uint64_t alignedScratchSize = AlignRayTracingBufferSize(
		    prebuildInfo.ScratchDataSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.ScratchBufferByteAlignment);
		entry.scratchBuffer =
		    m_renderHardwareInterface->GetRayTracingService().CreateRayTracingScratchBuffer(alignedScratchSize, L"RayTracingBlasScratch");
		entry.scratchBufferSizeInBytes = alignedScratchSize;
	}
	if (!entry.accelerationStructureBuffer)
	{
		const std::uint64_t alignedAccelerationStructureSize = AlignRayTracingBufferSize(
		    prebuildInfo.ResultDataMaxSizeInBytes,
		    m_renderHardwareInterface->GetCapabilities().RayTracing.AccelerationStructureByteAlignment);
		entry.accelerationStructureBuffer = m_renderHardwareInterface->GetRayTracingService().CreateRayTracingAccelerationStructureBuffer(
		    alignedAccelerationStructureSize,
		    ERhiRayTracingAccelerationStructureType::BottomLevel,
		    L"RayTracingBlas");
		entry.accelerationStructureSizeInBytes = alignedAccelerationStructureSize;
	}

	if (!entry.scratchBuffer || !entry.accelerationStructureBuffer)
	{
		SPDLOG_LOGGER_WARN(
		    g_rayTracingBlasCacheLogger,
		    "RayTracingBlasCache: failed to allocate BLAS resources for geometry (vertexCount={}, indexCount={}).",
		    geometry.VertexCount,
		    geometry.IndexCount);
		return false;
	}

	return true;
}

bool RayTracingBlasCache::GeometryMatches(const Entry& entry, const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	return GeometryEquals(entry.geometry, geometry);
}

RayTracingBlasCache::BlasHandle RayTracingBlasCache::BuildHandle(const Entry& entry) const noexcept
{
	if (m_renderHardwareInterface == nullptr || !entry.accelerationStructureBuffer)
	{
		return {};
	}

	return BlasHandle{
	    .resource = m_renderHardwareInterface->GetResourceService().GetNativeResource(entry.accelerationStructureBuffer),
	    .gpuAddress = m_renderHardwareInterface->GetResourceService().GetResourceGpuVirtualAddress(entry.accelerationStructureBuffer)};
}

