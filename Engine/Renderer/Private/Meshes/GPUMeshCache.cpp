#include "PCH.h"
#include "Meshes/GPUMeshCache.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Meshes/Mesh.h"

static const auto g_gpuMeshCacheLogger = Logging::GetOrCreateLogger("Renderer.GPUMeshCache");

GPUMeshCache::GPUMeshCache(RenderHardwareInterface& renderHardwareInterface) noexcept : m_renderHardwareInterface(&renderHardwareInterface)
{
}

GPUMesh* GPUMeshCache::GetOrUpload(const Mesh& cpuMesh)
{
	const Mesh* key = &cpuMesh;

	auto it = m_cache.find(key);
	if (it != m_cache.end())
	{
		return it->second.get();
	}

	auto gpuMesh = std::make_unique<GPUMesh>();
	if (!gpuMesh->Upload(*m_renderHardwareInterface, cpuMesh.GetMeshData()))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshCacheLogger, "[GPUMeshCache] Failed to upload mesh to GPU");
		return nullptr;
	}

	GPUMesh* result = gpuMesh.get();
	m_cache.emplace(key, std::move(gpuMesh));

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
		return it->second.get();
	}

	return nullptr;
}
