#pragma once

#include "RayTracing/RayTracingHitData.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

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
	RhiGpuDescriptorHandle GetVertexShaderResourceView() const noexcept { return m_vertexShaderResourceView; }
	RhiGpuDescriptorHandle GetSkinInfluenceShaderResourceView() const noexcept { return m_skinInfluenceShaderResourceView; }
	RhiGpuDescriptorHandle GetIndexShaderResourceView() const noexcept { return m_indexShaderResourceView; }
	RhiGpuDescriptorHandle GetInstanceShaderResourceView() const noexcept { return m_instanceShaderResourceView; }
	RhiGpuDescriptorHandle GetMaterialShaderResourceView() const noexcept { return m_materialShaderResourceView; }

	static RayTracingHitDataFrameData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	void Release() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_vertexBuffer = {};
	RhiOwnedResourceHandle m_skinInfluenceBuffer = {};
	RhiOwnedResourceHandle m_indexBuffer = {};
	RhiOwnedResourceHandle m_instanceBuffer = {};
	RhiOwnedResourceHandle m_materialBuffer = {};
	RhiResourceViewHandle m_vertexView = {};
	RhiResourceViewHandle m_skinInfluenceView = {};
	RhiResourceViewHandle m_indexView = {};
	RhiResourceViewHandle m_instanceView = {};
	RhiResourceViewHandle m_materialView = {};
	RhiGpuDescriptorHandle m_vertexShaderResourceView = {};
	RhiGpuDescriptorHandle m_skinInfluenceShaderResourceView = {};
	RhiGpuDescriptorHandle m_indexShaderResourceView = {};
	RhiGpuDescriptorHandle m_instanceShaderResourceView = {};
	RhiGpuDescriptorHandle m_materialShaderResourceView = {};
	std::uint32_t m_instanceCount = 0u;
	std::uint32_t m_materialCount = 0u;
};
