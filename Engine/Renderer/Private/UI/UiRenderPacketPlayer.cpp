#include "PCH.h"
#include "UI/UiRenderPacketPlayer.h"

#include "Editor/EditorTextureRegistry.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"

#include <imgui.h>

struct UiRenderPacketPlayer::PlaybackStorage final
{
	std::vector<std::unique_ptr<ImDrawList>> DrawLists;
	ImDrawData DrawData;
};

UiRenderPacketPlayer::UiRenderPacketPlayer() :
	m_storage(std::make_unique<PlaybackStorage>())
{
}

UiRenderPacketPlayer::~UiRenderPacketPlayer() noexcept = default;

void UiRenderPacketPlayer::Render(
    const UiRenderPacket& packet,
    const EditorTextureRegistry& textures,
    RhiImGuiRenderer& renderer)
{
	if (!packet.HasDrawData())
	{
		return;
	}

	PrepareDrawLists(packet.DrawLists.size());
	for (std::size_t drawListIndex = 0; drawListIndex < packet.DrawLists.size(); ++drawListIndex)
	{
		CopyDrawList(packet, packet.DrawLists[drawListIndex], drawListIndex, textures);
	}

	PrepareDrawData(packet);
	renderer.RenderDrawData(&m_storage->DrawData);
}

void UiRenderPacketPlayer::PrepareDrawLists(std::size_t drawListCount)
{
	while (m_storage->DrawLists.size() < drawListCount)
	{
		m_storage->DrawLists.push_back(std::make_unique<ImDrawList>(nullptr));
	}
}

void UiRenderPacketPlayer::CopyDrawList(
    const UiRenderPacket& packet,
    const UiDrawList& packetList,
    std::size_t drawListIndex,
    const EditorTextureRegistry& textures)
{
	ImDrawList& drawList = *m_storage->DrawLists[drawListIndex];
	drawList.VtxBuffer.resize(packetList.VertexCount);
	drawList.IdxBuffer.resize(packetList.IndexCount);
	drawList.CmdBuffer.resize(packetList.CommandCount);

	for (std::uint32_t index = 0; index < packetList.VertexCount; ++index)
	{
		const UiDrawVertex& source = packet.Vertices[packetList.VertexOffset + index];
		drawList.VtxBuffer[index] = ImDrawVert{
		    .pos = {source.Position[0], source.Position[1]},
		    .uv = {source.Uv[0], source.Uv[1]},
		    .col = source.Color};
	}
	for (std::uint32_t index = 0; index < packetList.IndexCount; ++index)
	{
		drawList.IdxBuffer[index] =
		    static_cast<ImDrawIdx>(packet.Indices[packetList.IndexOffset + index]);
	}
	for (std::uint32_t index = 0; index < packetList.CommandCount; ++index)
	{
		const UiDrawCommand& source = packet.Commands[packetList.CommandOffset + index];
		ImDrawCmd& command = drawList.CmdBuffer[index];
		command.ClipRect = {
		    source.ClipRect[0],
		    source.ClipRect[1],
		    source.ClipRect[2],
		    source.ClipRect[3]};
		command.TexRef = ImTextureRef(
		    static_cast<ImTextureID>(textures.Resolve(source.TextureHandle)));
		command.ElemCount = source.ElementCount;
		command.IdxOffset = source.IndexOffset;
		command.VtxOffset = static_cast<unsigned int>(source.VertexOffset);
		command.UserCallback =
		    source.Kind == UiDrawCommandKind::ResetRenderState
		        ? ImDrawCallback_ResetRenderState
		        : nullptr;
	}
}

void UiRenderPacketPlayer::PrepareDrawData(const UiRenderPacket& packet)
{
	ImDrawData& drawData = m_storage->DrawData;
	drawData.Valid = true;
	drawData.CmdLists.clear();
	drawData.CmdLists.reserve(static_cast<int>(packet.DrawLists.size()));
	for (std::size_t index = 0; index < packet.DrawLists.size(); ++index)
	{
		drawData.CmdLists.push_back(m_storage->DrawLists[index].get());
	}
	drawData.CmdListsCount = static_cast<int>(packet.DrawLists.size());
	drawData.TotalIdxCount = static_cast<int>(packet.Indices.size());
	drawData.TotalVtxCount = static_cast<int>(packet.Vertices.size());
	drawData.DisplayPos = {packet.DisplayPosition[0], packet.DisplayPosition[1]};
	drawData.DisplaySize = {packet.DisplaySize[0], packet.DisplaySize[1]};
	drawData.FramebufferScale = {
	    packet.FramebufferScale[0],
	    packet.FramebufferScale[1]};
}
