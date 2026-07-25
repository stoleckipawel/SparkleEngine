#pragma once

#include "Renderer/Public/Editor/EditorRenderPacket.h"

#include <functional>

struct ImDrawData;

class EditorRenderPacketBuilder final
{
  public:
	explicit EditorRenderPacketBuilder(
	    std::function<std::uint64_t(std::uint64_t)> registerTexture);
	EditorRenderPacket Build(const ImDrawData& drawData, std::uint64_t viewportGeneration);

  private:
	void Reserve(const ImDrawData& drawData);
	void AppendDrawList(const struct ImDrawList& drawList);

	EditorRenderPacket m_packet;
	std::function<std::uint64_t(std::uint64_t)> m_registerTexture;
};
