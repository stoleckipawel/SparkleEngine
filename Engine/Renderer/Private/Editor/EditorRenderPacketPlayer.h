#pragma once

#include "Renderer/Public/Editor/EditorRenderPacket.h"

class RhiImGuiRenderer;
class EditorTextureRegistry;

class EditorRenderPacketPlayer final
{
  public:
	void Render(
	    const EditorRenderPacket& packet,
	    const EditorTextureRegistry& textures,
	    RhiImGuiRenderer& renderer);
};
