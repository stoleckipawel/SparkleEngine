#pragma once

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

class RenderHardwareInterface;
struct RenderSceneData;

class SkinningFrameData final
{
  public:
	SkinningFrameData() noexcept = default;
	~SkinningFrameData() noexcept;

	SkinningFrameData(const SkinningFrameData&) = delete;
	SkinningFrameData& operator=(const SkinningFrameData&) = delete;
	SkinningFrameData(SkinningFrameData&& other) noexcept;
	SkinningFrameData& operator=(SkinningFrameData&& other) noexcept;

	bool IsValid() const noexcept { return static_cast<bool>(m_shaderResourceView); }
	RhiGpuDescriptorHandle GetShaderResourceView() const noexcept { return m_shaderResourceView; }

	static SkinningFrameData Build(RenderHardwareInterface& renderHardwareInterface, const RenderSceneData& sceneData);

  private:
	void Release() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_buffer = {};
	RhiResourceViewHandle m_view = {};
	RhiGpuDescriptorHandle m_shaderResourceView = {};
};
