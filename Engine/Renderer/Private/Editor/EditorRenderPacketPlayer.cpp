#include "PCH.h"
#include "Editor/EditorRenderPacketPlayer.h"

#include "Editor/EditorTextureRegistry.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"

#include <imgui.h>

void EditorRenderPacketPlayer::Render(
    const EditorRenderPacket& packet,
    const EditorTextureRegistry& textures,
    RhiImGuiRenderer& renderer)
{
	if (!packet.HasDrawData())
	{
		return;
	}

	std::vector<std::unique_ptr<ImDrawList>> ownedLists;
	std::vector<ImDrawList*> drawLists;
	ownedLists.reserve(packet.DrawLists.size());
	drawLists.reserve(packet.DrawLists.size());
	for (const EditorDrawList& packetList : packet.DrawLists)
	{
		auto drawList = std::make_unique<ImDrawList>(nullptr);
		drawList->VtxBuffer.resize(packetList.VertexCount);
		drawList->IdxBuffer.resize(packetList.IndexCount);
		drawList->CmdBuffer.resize(packetList.CommandCount);

		for (std::uint32_t index = 0; index < packetList.VertexCount; ++index)
		{
			const EditorDrawVertex& source = packet.Vertices[packetList.VertexOffset + index];
			drawList->VtxBuffer[index] = ImDrawVert{
			    .pos = {source.Position[0], source.Position[1]},
			    .uv = {source.Uv[0], source.Uv[1]},
			    .col = source.Color};
		}
		for (std::uint32_t index = 0; index < packetList.IndexCount; ++index)
		{
			drawList->IdxBuffer[index] = static_cast<ImDrawIdx>(packet.Indices[packetList.IndexOffset + index]);
		}
		for (std::uint32_t index = 0; index < packetList.CommandCount; ++index)
		{
			const EditorDrawCommand& source = packet.Commands[packetList.CommandOffset + index];
			ImDrawCmd& command = drawList->CmdBuffer[index];
			command.ClipRect = {source.ClipRect[0], source.ClipRect[1], source.ClipRect[2], source.ClipRect[3]};
			command.TexRef = ImTextureRef(
			    static_cast<ImTextureID>(textures.Resolve(source.TextureHandle)));
			command.ElemCount = source.ElementCount;
			command.IdxOffset = source.IndexOffset;
			command.VtxOffset = static_cast<unsigned int>(source.VertexOffset);
			command.UserCallback =
			    source.Kind == EditorDrawCommandKind::ResetRenderState ? ImDrawCallback_ResetRenderState : nullptr;
		}

		drawLists.push_back(drawList.get());
		ownedLists.push_back(std::move(drawList));
	}

	ImDrawData drawData;
	drawData.Valid = true;
	drawData.CmdListsCount = static_cast<int>(drawLists.size());
	drawData.TotalIdxCount = static_cast<int>(packet.Indices.size());
	drawData.TotalVtxCount = static_cast<int>(packet.Vertices.size());
	for (ImDrawList* drawList : drawLists)
	{
		drawData.CmdLists.push_back(drawList);
	}
	drawData.DisplayPos = {packet.DisplayPosition[0], packet.DisplayPosition[1]};
	drawData.DisplaySize = {packet.DisplaySize[0], packet.DisplaySize[1]};
	drawData.FramebufferScale = {packet.FramebufferScale[0], packet.FramebufferScale[1]};
	renderer.RenderDrawData(&drawData);
}
