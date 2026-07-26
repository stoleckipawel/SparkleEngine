#include "PCH.h"
#include "Renderer/Public/UI/ImGuiRenderPacketBuilder.h"

#include <imgui.h>

UiRenderPacket ImGuiRenderPacketBuilder::Build(
    const ImDrawData& drawData,
    UiPresentationMode presentationMode,
    std::uint64_t viewportGeneration)
{
	m_packet = {};
	m_packet.UiFrameId = ImGui::GetFrameCount();
	m_packet.ViewportGeneration = viewportGeneration;
	m_packet.PresentationMode = presentationMode;
	m_packet.DisplayPosition[0] = drawData.DisplayPos.x;
	m_packet.DisplayPosition[1] = drawData.DisplayPos.y;
	m_packet.DisplaySize[0] = drawData.DisplaySize.x;
	m_packet.DisplaySize[1] = drawData.DisplaySize.y;
	m_packet.FramebufferScale[0] = drawData.FramebufferScale.x;
	m_packet.FramebufferScale[1] = drawData.FramebufferScale.y;
	Reserve(drawData);

	for (const ImDrawList* drawList : drawData.CmdLists)
	{
		if (drawList != nullptr)
		{
			AppendDrawList(*drawList);
		}
	}
	return std::move(m_packet);
}

void ImGuiRenderPacketBuilder::Reserve(const ImDrawData& drawData)
{
	m_packet.Vertices.reserve(drawData.TotalVtxCount);
	m_packet.Indices.reserve(drawData.TotalIdxCount);
	m_packet.DrawLists.reserve(drawData.CmdListsCount);
	std::size_t commandCount = 0;
	for (const ImDrawList* drawList : drawData.CmdLists)
	{
		if (drawList != nullptr)
		{
			commandCount += drawList->CmdBuffer.size();
		}
	}
	m_packet.Commands.reserve(commandCount);
}

void ImGuiRenderPacketBuilder::AppendDrawList(const ImDrawList& drawList)
{
	UiDrawList packetList{
	    .VertexOffset = static_cast<std::uint32_t>(m_packet.Vertices.size()),
	    .VertexCount = static_cast<std::uint32_t>(drawList.VtxBuffer.size()),
	    .IndexOffset = static_cast<std::uint32_t>(m_packet.Indices.size()),
	    .IndexCount = static_cast<std::uint32_t>(drawList.IdxBuffer.size()),
	    .CommandOffset = static_cast<std::uint32_t>(m_packet.Commands.size()),
	    .CommandCount = static_cast<std::uint32_t>(drawList.CmdBuffer.size())};

	for (const ImDrawVert& vertex : drawList.VtxBuffer)
	{
		m_packet.Vertices.push_back(UiDrawVertex{
		    .Position = {vertex.pos.x, vertex.pos.y},
		    .Uv = {vertex.uv.x, vertex.uv.y},
		    .Color = vertex.col});
	}
	for (ImDrawIdx index : drawList.IdxBuffer)
	{
		m_packet.Indices.push_back(index);
	}
	for (const ImDrawCmd& command : drawList.CmdBuffer)
	{
		const std::uint64_t packedTextureHandle =
		    static_cast<std::uint64_t>(command.GetTexID());
		m_packet.Commands.push_back(UiDrawCommand{
		    .ClipRect = {command.ClipRect.x, command.ClipRect.y, command.ClipRect.z, command.ClipRect.w},
		    .TextureHandle = EditorTextureHandle::Unpack(packedTextureHandle),
		    .ElementCount = command.ElemCount,
		    .IndexOffset = command.IdxOffset,
		    .VertexOffset = static_cast<std::int32_t>(command.VtxOffset),
		    .Kind = command.UserCallback == ImDrawCallback_ResetRenderState
		                ? UiDrawCommandKind::ResetRenderState
		                : UiDrawCommandKind::Draw});
	}
	m_packet.DrawLists.push_back(packetList);
}
