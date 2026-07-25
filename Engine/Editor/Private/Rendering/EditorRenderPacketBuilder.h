#pragma once

#include "Renderer/Public/Editor/EditorRenderPacket.h"

struct ImDrawData;

class EditorRenderPacketBuilder final
{
  public:
	EditorRenderPacketBuilder() = default;
	EditorRenderPacket Build(const ImDrawData& drawData, std::uint64_t viewportGeneration);

  private:
	void Reserve(const ImDrawData& drawData);
	void AppendDrawList(const struct ImDrawList& drawList);

	EditorRenderPacket m_packet;
};
