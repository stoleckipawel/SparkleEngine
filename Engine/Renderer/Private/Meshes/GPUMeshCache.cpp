#include "PCH.h"
#include "Meshes/GPUMeshCache.h"

#include "Meshes/GPUMeshUploadDescBuilder.h"
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
	if (it != m_cache.end() && !cpuMesh.IsGeometryDirty())
	{
		return it->second.get();
	}

	auto gpuMesh = std::make_unique<GPUMesh>(AllocateHandle());
	if (!gpuMesh->Upload(*m_renderHardwareInterface, GPUMeshUploadDescBuilder::Build(cpuMesh)))
	{
		SPDLOG_LOGGER_ERROR(g_gpuMeshCacheLogger, "[GPUMeshCache] Failed to upload mesh to GPU");
		return nullptr;
	}

	GPUMesh* result = gpuMesh.get();
	m_cache.insert_or_assign(key, std::move(gpuMesh));

	return result;
}

GpuMeshHandle GPUMeshCache::AllocateHandle() noexcept
{
	const GpuMeshHandle handle{m_nextGpuMeshHandle++};
	if (m_nextGpuMeshHandle == 0u)
	{
		m_nextGpuMeshHandle = 1u;
	}
	return handle;
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
