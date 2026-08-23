#pragma once

#include "Renderer/Public/Editor/EditorTextureHandle.h"
#include "RHI/Public/Interop/ResourceState.h"

#include <cstdint>
#include <memory>

class EditorTextureRegistry;
class FrameGraph;
class RenderDeviceServices;
class UiRenderPacketPlayer;
struct UiRenderPacket;
class ViewportRenderProducts;

class UiFrameRenderer final
{
public:
	UiFrameRenderer(RenderDeviceServices& deviceServices, bool ownsBackend);
	~UiFrameRenderer() noexcept;

	UiFrameRenderer(const UiFrameRenderer&) = delete;
	UiFrameRenderer& operator=(const UiFrameRenderer&) = delete;

	EditorTextureHandle RegisterEditorTexture(std::uint64_t nativeTextureId) noexcept;
	void BeginFrame() noexcept;
	void Render(const UiRenderPacket& packet, FrameGraph* frameGraph, ViewportRenderProducts& viewportProducts) noexcept;

private:
	bool BeginViewportPresentation(FrameGraph& frameGraph, ViewportRenderProducts& viewportProducts) noexcept;
	void EndViewportPresentation(FrameGraph& frameGraph, const ViewportRenderProducts& viewportProducts) noexcept;
	void RenderEditorViewport(
	    const UiRenderPacket& packet,
	    FrameGraph* frameGraph,
	    ViewportRenderProducts& viewportProducts) noexcept;
	void RenderHostOverlay(const UiRenderPacket& packet) noexcept;
	void Play(const UiRenderPacket& packet) noexcept;
	void TransitionViewportProduct(
	    FrameGraph& frameGraph,
	    const ViewportRenderProducts& viewportProducts,
	    ResourceState resourceState) noexcept;

	RenderDeviceServices& m_deviceServices;
	std::unique_ptr<UiRenderPacketPlayer> m_packetPlayer;
	std::unique_ptr<EditorTextureRegistry> m_textureRegistry;
	bool m_ownsBackend = false;
};
