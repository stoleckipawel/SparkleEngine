#pragma once

#include "Renderer/Public/UI/UiRenderPacket.h"

#include <cstddef>
#include <memory>

class RhiImGuiRenderer;
class EditorTextureRegistry;

class UiRenderPacketPlayer final
{
  public:
	UiRenderPacketPlayer();
	~UiRenderPacketPlayer() noexcept;

	UiRenderPacketPlayer(const UiRenderPacketPlayer&) = delete;
	UiRenderPacketPlayer& operator=(const UiRenderPacketPlayer&) = delete;

	void Render(
	    const UiRenderPacket& packet,
	    const EditorTextureRegistry& textures,
	    RhiImGuiRenderer& renderer);

  private:
	struct PlaybackStorage;

	void PrepareDrawLists(std::size_t drawListCount);
	void CopyDrawList(
	    const UiRenderPacket& packet,
	    const UiDrawList& packetList,
	    std::size_t drawListIndex,
	    const EditorTextureRegistry& textures);
	void PrepareDrawData(const UiRenderPacket& packet);

	std::unique_ptr<PlaybackStorage> m_storage;
};
