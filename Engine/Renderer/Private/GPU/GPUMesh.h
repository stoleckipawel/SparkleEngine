#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cstdint>

class CommandContext;
struct MeshData;

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

	void Bind(CommandContext& cmd) const noexcept;

	std::uint32_t GetIndexCount() const noexcept { return m_indexCount; }
	std::uint32_t GetVertexCount() const noexcept { return m_vertexCount; }

	bool IsValid() const noexcept { return m_vertexBuffer && m_indexBuffer; }

	RhiVertexBufferView GetVertexBufferView() const noexcept;
	RhiIndexBufferView GetIndexBufferView() const noexcept;
	RhiOwnedResourceHandle GetVertexBufferResource() const noexcept { return m_vertexBuffer; }
	RhiOwnedResourceHandle GetIndexBufferResource() const noexcept { return m_indexBuffer; }
	RhiRayTracingGeometryDesc GetRayTracingGeometry() const noexcept;

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_vertexBuffer = {};
	RhiOwnedResourceHandle m_indexBuffer = {};
	RhiVertexBufferView m_vertexBufferView{};
	RhiIndexBufferView m_indexBufferView{};

	std::uint32_t m_vertexCount = 0;
	std::uint32_t m_indexCount = 0;
};
