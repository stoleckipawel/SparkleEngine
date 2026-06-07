#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphResourceHandle.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

#include <vector>

struct FrameGraphResourceAccess
{
	NativeResourceHandle resource = {};
	RhiGpuVirtualAddress accelerationStructureGpuAddress = 0;
	RhiResourceViewHandle renderTargetView = {};
	RhiResourceViewHandle depthStencilView = {};
	RhiResourceViewHandle shaderResourceView = {};
	RhiResourceViewHandle unorderedAccessView = {};

	bool IsResolved() const noexcept { return static_cast<bool>(resource); }
};

class FrameGraphResourceResolver final
{
  public:
	FrameGraphResourceResolver() = default;
	~FrameGraphResourceResolver() = default;

	FrameGraphResourceResolver(const FrameGraphResourceResolver&) = delete;
	FrameGraphResourceResolver& operator=(const FrameGraphResourceResolver&) = delete;
	FrameGraphResourceResolver(FrameGraphResourceResolver&&) = delete;
	FrameGraphResourceResolver& operator=(FrameGraphResourceResolver&&) = delete;

	void Clear() noexcept;
	void ClearResolvedAccess(FrameGraphResourceHandle handle) noexcept;
	void RegisterResource(FrameGraphResourceHandle handle, NativeResourceHandle resource) noexcept;
	void SetResolvedAccess(FrameGraphResourceHandle handle, const FrameGraphResourceAccess& access) noexcept;

	FrameGraphResourceAccess& GetResolvedAccess(FrameGraphResourceHandle handle) noexcept;
	const FrameGraphResourceAccess& GetResolvedAccess(FrameGraphResourceHandle handle) const noexcept;

  private:
	void EnsureStorage(FrameGraphResourceHandle handle) noexcept;

	std::vector<FrameGraphResourceAccess> m_resolvedAccessEntries;
};
