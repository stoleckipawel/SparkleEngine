#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Meshes/GPUSkinInfluenceBuffer.h"
#include "RayTracing/RTIndirectSpecularHitData.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

#include <DirectXMath.h>

#include <cstdint>
#include <span>
#include <vector>

class RenderCommandContext;
struct MeshData;
struct VertexSkinInfluence;

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

class SPARKLE_RENDERER_API GPUMesh final
{
  public:
	GPUMesh() = default;
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

	RhiVertexBufferView GetVertexBufferView() const noexcept;
	RhiIndexBufferView GetIndexBufferView() const noexcept;
	RhiOwnedResourceHandle GetVertexBufferResource() const noexcept { return m_vertexBuffer; }
	RhiOwnedResourceHandle GetIndexBufferResource() const noexcept { return m_indexBuffer; }
	RhiGpuDescriptorHandle GetSkinInfluencesShaderResourceView() const noexcept { return m_skinInfluences.GetShaderResourceView(); }
	RhiRayTracingGeometryDesc GetRayTracingGeometry() const noexcept;
	const GPUMeshBounds& GetLocalBounds() const noexcept { return m_localBounds; }
	bool HasRTIndirectSpecularHitData() const noexcept { return !m_rtIndirectSpecularHitVertices.empty() && !m_rtIndirectSpecularHitIndices.empty(); }
	std::span<const RTIndirectSpecularHitVertex> GetRTIndirectSpecularHitVertices() const noexcept { return m_rtIndirectSpecularHitVertices; }
	std::span<const std::uint32_t> GetRTIndirectSpecularHitIndices() const noexcept { return m_rtIndirectSpecularHitIndices; }

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_vertexBuffer = {};
	RhiOwnedResourceHandle m_indexBuffer = {};
	GPUSkinInfluenceBuffer m_skinInfluences;
	RhiVertexBufferView m_vertexBufferView{};
	RhiIndexBufferView m_indexBufferView{};

	std::uint32_t m_vertexCount = 0;
	std::uint32_t m_indexCount = 0;
	GPUMeshBounds m_localBounds = {};
	std::vector<RTIndirectSpecularHitVertex> m_rtIndirectSpecularHitVertices;
	std::vector<std::uint32_t> m_rtIndirectSpecularHitIndices;
};
