#include "PCH.h"
#include "Renderer/Public/UI/UiRenderPacket.h"

bool UiRenderPacket::HasDrawData() const noexcept
{
	return DisplaySize[0] > 0.0f && DisplaySize[1] > 0.0f && !DrawLists.empty();
}
