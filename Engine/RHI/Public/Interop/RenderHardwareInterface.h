#pragma once

#include "../RHIAPI.h"

#include <cstddef>
#include <cstdint>

enum class RhiBackendApi : std::uint8_t
{
	Unknown = 0,
	D3D12 = 1,
	Vulkan = 2,
};

struct NativeGraphicsDeviceHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeGraphicsQueueHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeGraphicsCommandListHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeDescriptorHeapHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct NativeTextureHandle
{
	void* Value = nullptr;

	constexpr explicit operator bool() const noexcept { return Value != nullptr; }
};

struct RhiCpuDescriptorHandle
{
	std::uintptr_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

struct RhiGpuDescriptorHandle
{
	std::uint64_t Value = 0;

	constexpr explicit operator bool() const noexcept { return Value != 0; }
};

class SPARKLE_RHI_API RenderHardwareInterface
{
  public:
	virtual ~RenderHardwareInterface() noexcept = default;

	virtual RhiBackendApi GetBackendApi() const noexcept = 0;
	virtual std::uint32_t GetCurrentFrameIndex() const noexcept = 0;
	virtual NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept = 0;
	virtual NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept = 0;
	virtual NativeGraphicsCommandListHandle GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept = 0;
	virtual NativeDescriptorHeapHandle GetShaderResourceHeapHandle() const noexcept = 0;
	virtual void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) = 0;
	virtual void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept = 0;
	virtual void BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4]) const noexcept = 0;
	virtual void EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept = 0;
	virtual std::uint32_t GetPresentColorFormat() const noexcept = 0;
};