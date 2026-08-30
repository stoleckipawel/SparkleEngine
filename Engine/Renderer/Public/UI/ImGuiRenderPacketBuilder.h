#pragma once

#include "UiRenderPacket.h"

struct ImDrawData;
struct ImDrawList;
struct ImTextureData;

class SPARKLE_RENDERER_API ImGuiRenderPacketBuilder final
{
public:
	static void ConfigureProducerContext() noexcept;

	UiRenderPacket Build(const ImDrawData& drawData, UiPresentationMode presentationMode, std::uint64_t viewportGeneration = 0);

private:
	void Reserve(const ImDrawData& drawData);
	void AppendTextureUpdates(const ImDrawData& drawData);
	bool AppendTextureUpload(ImTextureData& texture, EditorTextureHandle handle);
	void AppendDrawList(const ImDrawList& drawList);

	UiRenderPacket m_packet;
};
