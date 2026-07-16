#pragma once

#include "Meshes/GPUSkinInfluenceBuffer.h"
#include "RayTracing/RayTracingHitData.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/RayTracing/RhiRayTracingDesc.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"
#include "Scene/Meshes/MeshSkinningData.h"

#include <DirectXMath.h>

#include <cstdint>
#include <span>
#include <vector>

class RenderCommandContext;
class RenderHardwareInterface;
struct MeshData;
struct GpuMeshHandle final
{
	std::uint64_t Value = 0u;

	constexpr explicit operator bool() const noexcept { return Value != 0u; }
	constexpr bool operator==(const GpuMeshHandle&) const noexcept = default;
};

struct GPUMeshBounds final
{
	DirectX::XMFLOAT3 Min = {};
	DirectX::XMFLOAT3 Max = {};
	bool Valid = false;
};

struct GPUMeshUploadDesc
{
	const MeshData& meshData;
	std::span<const VertexSkinInfluence> skinInfluences = {};
};

class GPUMesh final
{
  public:
	explicit GPUMesh(GpuMeshHandle handle = {}) noexcept : m_handle(handle) {}
	~GPUMesh() noexcept;

	GPUMesh(const GPUMesh&) = delete;
	GPUMesh& operator=(const GPUMesh&) = delete;
	GPUMesh(GPUMesh&&) = delete;
	GPUMesh& operator=(GPUMesh&&) = delete;

	bool Upload(RenderHardwareInterface& renderHardwareInterface, const MeshData& meshData);
	bool Upload(RenderHardwareInterface& renderHardwareInterface, const GPUMeshUploadDesc& uploadDesc);

	void Bind(RenderCommandContext& cmd) const noexcept;

	std::uint32_t GetIndexCount() const noexcept { return m_indexCount; }
	std::uint32_t GetVertexCount() const noexcept { return m_vertexCount; }

	bool IsValid() const noexcept { return m_vertexBuffer && m_indexBuffer; }
	GpuMeshHandle GetHandle() const noexcept { return m_handle; }

	RhiVertexBufferView GetVertexBufferView() const noexcept;
	RhiIndexBufferView GetIndexBufferView() const noexcept;
	RhiOwnedResourceHandle GetVertexBufferResource() const noexcept { return m_vertexBuffer; }
	RhiOwnedResourceHandle GetIndexBufferResource() const noexcept { return m_indexBuffer; }
	RhiGpuDescriptorHandle GetSkinInfluencesShaderResourceView() const noexcept { return m_skinInfluences.GetShaderResourceView(); }
	RhiRayTracingGeometryDesc GetRayTracingGeometry() const noexcept;
	const GPUMeshBounds& GetLocalBounds() const noexcept { return m_localBounds; }
	bool HasRayTracingHitData() const noexcept { return !m_rayTracingHitVertices.empty() && !m_rayTracingHitIndices.empty(); }
	std::span<const RayTracingHitVertex> GetRayTracingHitVertices() const noexcept { return m_rayTracingHitVertices; }
	std::span<const std::uint32_t> GetRayTracingHitIndices() const noexcept { return m_rayTracingHitIndices; }
	bool HasSkinInfluences() const noexcept { return m_cpuSkinInfluences.size() == m_vertexCount; }
	std::span<const VertexSkinInfluence> GetSkinInfluences() const noexcept { return m_cpuSkinInfluences; }

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	GpuMeshHandle m_handle = {};
	RhiOwnedResourceHandle m_vertexBuffer = {};
	RhiOwnedResourceHandle m_indexBuffer = {};
	GPUSkinInfluenceBuffer m_skinInfluences;
	RhiVertexBufferView m_vertexBufferView{};
	RhiIndexBufferView m_indexBufferView{};

	std::uint32_t m_vertexCount = 0;
	std::uint32_t m_indexCount = 0;
	GPUMeshBounds m_localBounds = {};
	std::vector<RayTracingHitVertex> m_rayTracingHitVertices;
	std::vector<std::uint32_t> m_rayTracingHitIndices;
	std::vector<VertexSkinInfluence> m_cpuSkinInfluences;
};
