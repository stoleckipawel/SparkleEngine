#pragma once

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

class RenderHardwareInterface;
struct RenderSceneData;

class MeshInstanceFrameData final
{
  public:
	MeshInstanceFrameData() noexcept = default;
	~MeshInstanceFrameData() noexcept;

	MeshInstanceFrameData(const MeshInstanceFrameData&) = delete;
	MeshInstanceFrameData& operator=(const MeshInstanceFrameData&) = delete;
	MeshInstanceFrameData(MeshInstanceFrameData&& other) noexcept;
	MeshInstanceFrameData& operator=(MeshInstanceFrameData&& other) noexcept;

	bool IsValid() const noexcept { return static_cast<bool>(m_shaderResourceView); }
	RhiGpuDescriptorHandle GetShaderResourceView() const noexcept { return m_shaderResourceView; }

	static MeshInstanceFrameData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	void Release() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_buffer = {};
	RhiResourceViewHandle m_view = {};
	RhiGpuDescriptorHandle m_shaderResourceView = {};
};
