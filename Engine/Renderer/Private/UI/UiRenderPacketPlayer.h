#pragma once

#include "Renderer/Public/UI/UiRenderPacket.h"

#include <cstddef>
#include <memory>

class RhiImGuiRenderer;
class UiTextureRegistry;
struct ImTextureData;

class UiRenderPacketPlayer final
{
public:
	UiRenderPacketPlayer();
	~UiRenderPacketPlayer() noexcept;

	UiRenderPacketPlayer(const UiRenderPacketPlayer&) = delete;
	UiRenderPacketPlayer& operator=(const UiRenderPacketPlayer&) = delete;

	void Render(const UiRenderPacket& packet, const UiTextureRegistry& textures, RhiImGuiRenderer& renderer);
	void Shutdown(RhiImGuiRenderer& renderer) noexcept;

private:
	struct PlaybackStorage;

	void ApplyTextureUpdates(const UiRenderPacket& packet);
	void ApplyTextureUpload(const UiRenderPacket& packet, const UiTextureUpload& upload);
	void QueueTextureRelease(UiTextureHandle handle) noexcept;
	ImTextureData* FindTexture(UiTextureHandle handle) const noexcept;
	void RetireReleasedTextures() noexcept;
	void PrepareDrawLists(std::size_t drawListCount);
	void CopyDrawList(
	    const UiRenderPacket& packet,
	    const UiDrawList& packetList,
	    std::size_t drawListIndex,
	    const UiTextureRegistry& textures);
	void PrepareDrawData(const UiRenderPacket& packet);

	std::unique_ptr<PlaybackStorage> m_storage;
};
