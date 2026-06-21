#pragma once

#include "RayTracing/RTIndirectSpecularHitData.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

#include <cstdint>

class RenderHardwareInterface;
struct RenderSceneData;

class RTIndirectSpecularHitDataFrameData final
{
  public:
	RTIndirectSpecularHitDataFrameData() noexcept = default;
	~RTIndirectSpecularHitDataFrameData() noexcept;

	RTIndirectSpecularHitDataFrameData(const RTIndirectSpecularHitDataFrameData&) = delete;
	RTIndirectSpecularHitDataFrameData& operator=(const RTIndirectSpecularHitDataFrameData&) = delete;
	RTIndirectSpecularHitDataFrameData(RTIndirectSpecularHitDataFrameData&& other) noexcept;
	RTIndirectSpecularHitDataFrameData& operator=(RTIndirectSpecularHitDataFrameData&& other) noexcept;

	bool IsValid() const noexcept;
	std::uint32_t GetInstanceCount() const noexcept { return m_instanceCount; }
	std::uint32_t GetMaterialCount() const noexcept { return m_materialCount; }
	RhiGpuDescriptorHandle GetVertexShaderResourceView() const noexcept { return m_vertexShaderResourceView; }
	RhiGpuDescriptorHandle GetIndexShaderResourceView() const noexcept { return m_indexShaderResourceView; }
	RhiGpuDescriptorHandle GetInstanceShaderResourceView() const noexcept { return m_instanceShaderResourceView; }
	RhiGpuDescriptorHandle GetMaterialShaderResourceView() const noexcept { return m_materialShaderResourceView; }

	static RTIndirectSpecularHitDataFrameData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	void Release() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_vertexBuffer = {};
	RhiOwnedResourceHandle m_indexBuffer = {};
	RhiOwnedResourceHandle m_instanceBuffer = {};
	RhiOwnedResourceHandle m_materialBuffer = {};
	RhiResourceViewHandle m_vertexView = {};
	RhiResourceViewHandle m_indexView = {};
	RhiResourceViewHandle m_instanceView = {};
	RhiResourceViewHandle m_materialView = {};
	RhiGpuDescriptorHandle m_vertexShaderResourceView = {};
	RhiGpuDescriptorHandle m_indexShaderResourceView = {};
	RhiGpuDescriptorHandle m_instanceShaderResourceView = {};
	RhiGpuDescriptorHandle m_materialShaderResourceView = {};
	std::uint32_t m_instanceCount = 0u;
	std::uint32_t m_materialCount = 0u;
};

