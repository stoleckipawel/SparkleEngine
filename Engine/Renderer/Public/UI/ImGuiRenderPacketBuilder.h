#pragma once

#include "UiRenderPacket.h"

struct ImDrawData;
struct ImDrawList;

class SPARKLE_RENDERER_API ImGuiRenderPacketBuilder final
{
  public:
	UiRenderPacket Build(
	    const ImDrawData& drawData,
	    UiPresentationMode presentationMode,
	    std::uint64_t viewportGeneration = 0);

  private:
	void Reserve(const ImDrawData& drawData);
	void AppendDrawList(const ImDrawList& drawList);

	UiRenderPacket m_packet;
};
