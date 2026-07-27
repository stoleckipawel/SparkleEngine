#pragma once

#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"
#include "ShaderData/MorphTargetShaderData.h"

#include <cstdint>
#include <span>
#include <vector>

class RenderHardwareInterface;
struct MeshMorphData;

// Owns one immutable mesh generation's morph deltas in CPU and GPU forms.
class GpuMorphTargetBuffer final
{
  public:
	GpuMorphTargetBuffer() noexcept;
	~GpuMorphTargetBuffer() noexcept;

	GpuMorphTargetBuffer(const GpuMorphTargetBuffer&) = delete;
	GpuMorphTargetBuffer& operator=(const GpuMorphTargetBuffer&) = delete;
	GpuMorphTargetBuffer(GpuMorphTargetBuffer&&) = delete;
	GpuMorphTargetBuffer& operator=(GpuMorphTargetBuffer&&) = delete;

	bool Upload(
	    RenderHardwareInterface& renderHardwareInterface,
	    std::vector<MorphTargetDeltaData> deltas,
	    std::uint32_t targetCount);
	void Release() noexcept;

	RhiGpuDescriptorHandle GetShaderResourceView() const noexcept { return m_shaderResourceView; }
	std::span<const MorphTargetDeltaData> GetDeltas() const noexcept { return m_deltas; }
	std::uint32_t GetTargetCount() const noexcept { return m_targetCount; }
	bool HasTargets() const noexcept { return m_targetCount > 0u; }

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiOwnedResourceHandle m_buffer = {};
	RhiResourceViewHandle m_view = {};
	RhiGpuDescriptorHandle m_shaderResourceView = {};
	std::vector<MorphTargetDeltaData> m_deltas;
	std::uint32_t m_targetCount = 0u;
};
