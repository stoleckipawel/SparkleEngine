#include "PCH.h"
#include "Renderer/Public/Editor/EditorRenderPacket.h"

bool EditorRenderPacket::HasDrawData() const noexcept
{
	return DisplaySize[0] > 0.0f && DisplaySize[1] > 0.0f && !DrawLists.empty();
}
