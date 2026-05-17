#pragma once

#include "../Descriptors/RhiDescriptorHandles.h"
#include "../RHIAPI.h"

#include <cstdint>

class RenderHardwareInterface;

struct RenderBindingSetDesc
{
	ERhiDescriptorAllocatorType DescriptorType = ERhiDescriptorAllocatorType::ShaderResource;
	std::uint32_t DescriptorCount = 0;
};

class SPARKLE_RHI_API RenderBindingSet final
{
  public:
	RenderBindingSet(RenderHardwareInterface& renderHardwareInterface, const RenderBindingSetDesc& desc) noexcept;
	~RenderBindingSet() noexcept;

	RenderBindingSet(const RenderBindingSet&) = delete;
	RenderBindingSet& operator=(const RenderBindingSet&) = delete;
	RenderBindingSet(RenderBindingSet&& other) noexcept;
	RenderBindingSet& operator=(RenderBindingSet&& other) noexcept;

	bool IsValid() const noexcept { return static_cast<bool>(m_tableHandle); }
	explicit operator bool() const noexcept { return IsValid(); }

	std::uint32_t GetDescriptorCount() const noexcept { return m_descriptorCount; }
	RhiCpuDescriptorHandle GetCpuDescriptorHandle(std::uint32_t descriptorIndex = 0) const noexcept;
	RhiDescriptorTableBinding GetTableBinding(std::uint32_t descriptorIndex = 0) const noexcept;

  private:
	void Reset() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	RhiDescriptorTableHandle m_tableHandle = {};
	std::uint32_t m_descriptorCount = 0;
};