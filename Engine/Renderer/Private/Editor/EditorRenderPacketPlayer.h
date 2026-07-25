#pragma once

#include "Renderer/Public/Editor/EditorRenderPacket.h"

#include <cstddef>
#include <memory>

class RhiImGuiRenderer;
class EditorTextureRegistry;

class EditorRenderPacketPlayer final
{
  public:
	EditorRenderPacketPlayer();
	~EditorRenderPacketPlayer() noexcept;

	EditorRenderPacketPlayer(const EditorRenderPacketPlayer&) = delete;
	EditorRenderPacketPlayer& operator=(const EditorRenderPacketPlayer&) = delete;

	void Render(
	    const EditorRenderPacket& packet,
	    const EditorTextureRegistry& textures,
	    RhiImGuiRenderer& renderer);

  private:
	struct PlaybackStorage;

	void PrepareDrawLists(std::size_t drawListCount);
	void CopyDrawList(
	    const EditorRenderPacket& packet,
	    const EditorDrawList& packetList,
	    std::size_t drawListIndex,
	    const EditorTextureRegistry& textures);
	void PrepareDrawData(const EditorRenderPacket& packet);

	std::unique_ptr<PlaybackStorage> m_storage;
};
