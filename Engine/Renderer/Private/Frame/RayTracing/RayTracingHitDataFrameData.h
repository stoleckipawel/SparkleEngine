#pragma once

#include "RayTracing/RayTracingHitData.h"
#include "Frame/Core/FrameBufferResource.h"

#include <cstdint>

class RenderHardwareInterface;
struct RenderSceneData;

class RayTracingHitDataFrameData final
{
  public:
	RayTracingHitDataFrameData() noexcept = default;
	~RayTracingHitDataFrameData() noexcept;

	RayTracingHitDataFrameData(const RayTracingHitDataFrameData&) = delete;
	RayTracingHitDataFrameData& operator=(const RayTracingHitDataFrameData&) = delete;
	RayTracingHitDataFrameData(RayTracingHitDataFrameData&& other) noexcept;
	RayTracingHitDataFrameData& operator=(RayTracingHitDataFrameData&& other) noexcept;

	bool IsValid() const noexcept;
	std::uint32_t GetInstanceCount() const noexcept { return m_instanceCount; }
	std::uint32_t GetMaterialCount() const noexcept { return m_materialCount; }
	const FrameBufferResource& GetVertexBuffer() const noexcept { return m_vertexBuffer; }
	const FrameBufferResource& GetSkinInfluenceBuffer() const noexcept { return m_skinInfluenceBuffer; }
	const FrameBufferResource& GetIndexBuffer() const noexcept { return m_indexBuffer; }
	const FrameBufferResource& GetInstanceBuffer() const noexcept { return m_instanceBuffer; }
	const FrameBufferResource& GetMaterialBuffer() const noexcept { return m_materialBuffer; }

	static RayTracingHitDataFrameData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	void Release() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	FrameBufferResource m_vertexBuffer;
	FrameBufferResource m_skinInfluenceBuffer;
	FrameBufferResource m_indexBuffer;
	FrameBufferResource m_instanceBuffer;
	FrameBufferResource m_materialBuffer;
	std::uint32_t m_instanceCount = 0u;
	std::uint32_t m_materialCount = 0u;
};
