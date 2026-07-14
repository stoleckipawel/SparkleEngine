#pragma once

#include "Frame/RhiFrameConstants.h"
#include "Resources/History/FrameHistory.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Formats/PixelFormat.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

class FrameGraph;
class FrameGraphBuilder;
class RenderHardwareInterface;

struct PersistentTextureHistorySpec final
{
	std::string_view Name;
	PixelFormat Format = PixelFormat::Unknown;
};

class PersistentTextureHistory final
{
  public:
	PersistentTextureHistory(RenderHardwareInterface& renderHardwareInterface, PersistentTextureHistorySpec spec);
	PersistentTextureHistory(RenderHardwareInterface& renderHardwareInterface, std::string_view name, PixelFormat format);
	~PersistentTextureHistory() noexcept;

	PersistentTextureHistory(const PersistentTextureHistory&) = delete;
	PersistentTextureHistory& operator=(const PersistentTextureHistory&) = delete;
	PersistentTextureHistory(PersistentTextureHistory&&) = delete;
	PersistentTextureHistory& operator=(PersistentTextureHistory&&) = delete;

	static FrameGraphTextureHistoryHandles Reserve(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent extent,
	    const PersistentTextureHistorySpec& spec);
	static FrameGraphTextureHistoryHandles Reserve(
	    FrameGraphBuilder& builder,
	    RenderViewportExtent extent,
	    std::string_view name,
	    PixelFormat format);

	void SetGraphHandles(FrameGraphTextureHistoryHandles handles) noexcept;
	void Configure(bool active, RenderViewportExtent extent);
	bool SetSemanticKey(std::uint64_t key) noexcept;
	void Invalidate() noexcept { m_valid = false; }
	void Bind(FrameGraph& frameGraph, std::uint32_t frameIndex, std::uint64_t resetGeneration) noexcept;
	void CommitFrame() noexcept;
	bool IsValid() const noexcept { return m_valid; }

  private:
	void Release() noexcept;
	bool HasResources() const noexcept;

	RenderHardwareInterface& m_renderHardwareInterface;
	std::wstring m_resourceName;
	PixelFormat m_format = PixelFormat::Unknown;
	FrameGraphTextureHistoryHandles m_graphHandles = {};
	std::array<RhiOwnedResourceHandle, RhiFrameConstants::FramesInFlight> m_resources = {};
	RenderViewportExtent m_extent = {};
	std::uint64_t m_semanticKey = 0;
	std::uint64_t m_consumedResetGeneration = 0;
	bool m_active = false;
	bool m_valid = false;
};
