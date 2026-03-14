#include "PCH.h"
#include "Renderer/Public/GPU/GPUMeshCache.h"

#include "D3D12Rhi.h"
#include "Scene/Mesh.h"
#include "Log.h"

GPUMeshCache::GPUMeshCache(D3D12Rhi& rhi) noexcept : m_rhi(&rhi) {}

GPUMesh* GPUMeshCache::GetOrUpload(const Mesh& cpuMesh)
{
	const Mesh* key = &cpuMesh;

	auto it = m_cache.find(key);
	if (it != m_cache.end())
	{
		return it->second.get();
	}

	auto gpuMesh = std::make_unique<GPUMesh>();
	if (!gpuMesh->Upload(*m_rhi, cpuMesh.GetMeshData()))
	{
		LOG_ERROR("[GPUMeshCache] Failed to upload mesh to GPU");
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
