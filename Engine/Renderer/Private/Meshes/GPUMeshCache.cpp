#include "PCH.h"
#include "Meshes/GPUMeshCache.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/SkeletalCookedMesh.h"

static const auto g_gpuMeshCacheLogger = Logging::GetOrCreateLogger("Renderer.GPUMeshCache");

namespace
{
	GPUMeshUploadDesc BuildUploadDesc(const Mesh& cpuMesh)
	{
		GPUMeshUploadDesc uploadDesc{.meshData = cpuMesh.GetMeshData()};
		if (const auto* skeletalMesh = dynamic_cast<const SkeletalCookedMesh*>(&cpuMesh))
		{
			uploadDesc.skinInfluences = skeletalMesh->GetSkeletalMeshData().skinInfluences;
		}
		return uploadDesc;
	}
}

GPUMeshCache::GPUMeshCache(RenderHardwareInterface& renderHardwareInterface) noexcept : m_renderHardwareInterface(&renderHardwareInterface)
{
}

GPUMesh* GPUMeshCache::GetOrUpload(const Mesh& cpuMesh)
{
	const Mesh* key = &cpuMesh;

	auto it = m_cache.find(key);
	const std::uint64_t geometryRevision = cpuMesh.GetGeometryRevision();
	if (it != m_cache.end() && it->second.GeometryRevision == geometryRevision)
	{
		return it->second.Mesh.get();
	}

	auto gpuMesh = std::make_unique<GPUMesh>();
	if (!gpuMesh->Upload(*m_renderHardwareInterface, BuildUploadDesc(cpuMesh)))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshCacheLogger, "[GPUMeshCache] Failed to upload mesh to GPU");
		return nullptr;
	}

	GPUMesh* result = gpuMesh.get();
	m_cache.insert_or_assign(key, CacheEntry{.Mesh = std::move(gpuMesh), .GeometryRevision = geometryRevision});

	return result;
}

void GPUMeshCache::Clear() noexcept
{
	m_cache.clear();
}

bool GPUMeshCache::Contains(const Mesh& cpuMesh) const noexcept
{
	return m_cache.contains(&cpuMesh);
}

const GPUMesh* GPUMeshCache::Find(const Mesh& cpuMesh) const noexcept
{
	if (auto it = m_cache.find(&cpuMesh); it != m_cache.end())
	{
		return it->second.Mesh.get();
	}

	return nullptr;
}
