#include "PCH.h"
#include "Meshes/GPUMeshCache.h"

#include "Meshes/GPUMeshUploadDescBuilder.h"
#include "GameFramework/Public/Rendering/RenderAssetHandles.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Meshes/Mesh.h"

static const auto g_gpuMeshCacheLogger = Logging::GetOrCreateLogger("Renderer.GPUMeshCache");

GPUMeshCache::GPUMeshCache(RenderHardwareInterface& renderHardwareInterface) noexcept : m_renderHardwareInterface(&renderHardwareInterface)
{
}

GPUMeshCache::~GPUMeshCache() noexcept = default;

GPUMesh* GPUMeshCache::GetOrUpload(const ImmutableRenderMeshHandle& mesh)
{
	if (!mesh.IsValid())
	{
		return nullptr;
	}

	const Mesh& cpuMesh = *mesh.GetResource();
	const CacheKey key{mesh.GetAssetId(), mesh.GetGeneration()};
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
	if (it != m_cache.end() && it->second != nullptr)
	{
		m_handles.erase(it->second->GetHandle().Value);
	}
	m_cache.insert_or_assign(key, std::move(gpuMesh));
	m_handles[result->GetHandle().Value] = result;
	m_sourceHandles[&cpuMesh] = result->GetHandle();

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
	m_sourceHandles.clear();
	m_handles.clear();
	m_cache.clear();
}

bool GPUMeshCache::Contains(const Mesh& cpuMesh) const noexcept
{
	return m_sourceHandles.contains(&cpuMesh);
}

const GPUMesh* GPUMeshCache::Find(const Mesh& cpuMesh) const noexcept
{
	const auto source = m_sourceHandles.find(&cpuMesh);
	return source != m_sourceHandles.end() ? Resolve(source->second) : nullptr;
}

const GPUMesh* GPUMeshCache::Resolve(GpuMeshHandle handle) const noexcept
{
	if (!handle)
	{
		return nullptr;
	}

	const auto mesh = m_handles.find(handle.Value);
	return mesh != m_handles.end() ? mesh->second : nullptr;
}
