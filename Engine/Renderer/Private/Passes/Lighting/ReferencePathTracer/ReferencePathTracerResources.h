#pragma once

#include "FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"

#include <cstdint>

class FrameGraph;
class FrameGraphBuilder;
class RendererMemoryMonitor;
class RenderDeviceServices;
class ReferencePathTracerSession;

struct ReferencePathTracerGraphResources final
{
	FrameGraphTextureHandle WorkingMean = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle WorkingM2 = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CommittedMean = FrameGraphTextureHandle::Invalid();
	FrameGraphTextureHandle CommittedM2 = FrameGraphTextureHandle::Invalid();
};

class ReferencePathTracerResources final
{
private:
	friend class ReferencePathTracerSession;

	ReferencePathTracerResources(RenderDeviceServices& deviceServices, RendererMemoryMonitor& memoryMonitor) noexcept;
	~ReferencePathTracerResources() noexcept;

	ReferencePathTracerResources(const ReferencePathTracerResources&) = delete;
	ReferencePathTracerResources& operator=(const ReferencePathTracerResources&) = delete;

	void ReserveGraphResources(FrameGraphBuilder& builder, RenderViewportExtent extent);
	void Allocate(RenderViewportExtent extent);
	void Release() noexcept;
	bool Bind(FrameGraph& frameGraph) const noexcept;
	void RecordUse() noexcept;
	bool CanRetain() const noexcept;
	bool IsAllocated() const noexcept;

	const ReferencePathTracerGraphResources& GetGraphResources() const noexcept { return m_graphResources; }
	struct Allocation final
	{
		RhiOwnedResourceHandle WorkingMean = {};
		RhiOwnedResourceHandle WorkingM2 = {};
		RhiOwnedResourceHandle CommittedMean = {};
		RhiOwnedResourceHandle CommittedM2 = {};
		RenderViewportExtent Extent = {};
		std::uint64_t Bytes = 0u;

		explicit operator bool() const noexcept { return static_cast<bool>(WorkingMean); }
	};

	RenderDeviceServices& m_deviceServices;
	RendererMemoryMonitor& m_memoryMonitor;
	ReferencePathTracerGraphResources m_graphResources;
	Allocation m_allocation;
	bool m_used = false;
};
