#include "PCH.h"
#include "Renderer/Public/UI/ImGuiRenderPacketBuilder.h"

#include <imgui.h>

void ImGuiRenderPacketBuilder::ConfigureProducerContext() noexcept
{
	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	io.BackendRendererName = "Sparkle.UiRenderPacket";
}

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
	AppendTextureUpdates(drawData);
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

void ImGuiRenderPacketBuilder::AppendTextureUpdates(const ImDrawData& drawData)
{
	if (drawData.Textures == nullptr)
	{
		return;
	}

	for (ImTextureData* texture : *drawData.Textures)
	{
		if (texture == nullptr || texture->UniqueID <= 0)
		{
			continue;
		}

		const EditorTextureHandle handle = EditorTextureHandle::ImGuiTexture(static_cast<std::uint32_t>(texture->UniqueID));
		switch (texture->Status)
		{
			case ImTextureStatus_WantCreate:
			case ImTextureStatus_WantUpdates:
				if (AppendTextureUpload(*texture, handle))
				{
					texture->SetTexID(static_cast<ImTextureID>(handle.Pack()));
					texture->SetStatus(ImTextureStatus_OK);
				}
				break;
			case ImTextureStatus_WantDestroy:
				m_packet.TextureReleases.push_back(handle);
				texture->SetTexID(ImTextureID_Invalid);
				texture->SetStatus(ImTextureStatus_Destroyed);
				break;
			default:
				break;
		}
	}
}

bool ImGuiRenderPacketBuilder::AppendTextureUpload(ImTextureData& texture, EditorTextureHandle handle)
{
	if (texture.Pixels == nullptr || texture.Width <= 0 || texture.Height <= 0 || texture.BytesPerPixel <= 0)
	{
		return false;
	}

	const std::size_t pixelCount = static_cast<std::size_t>(texture.Width) * static_cast<std::size_t>(texture.Height);
	const std::size_t pixelOffset = m_packet.TexturePixels.size();
	if (texture.Format == ImTextureFormat_Alpha8)
	{
		m_packet.TexturePixels.reserve(pixelOffset + (pixelCount * 4));
		for (std::size_t pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex)
		{
			m_packet.TexturePixels.push_back(std::byte{0xff});
			m_packet.TexturePixels.push_back(std::byte{0xff});
			m_packet.TexturePixels.push_back(std::byte{0xff});
			m_packet.TexturePixels.push_back(static_cast<std::byte>(texture.Pixels[pixelIndex]));
		}
	}
	else
	{
		const std::byte* pixels = reinterpret_cast<const std::byte*>(texture.Pixels);
		m_packet.TexturePixels.insert(m_packet.TexturePixels.end(), pixels, pixels + (pixelCount * 4));
	}

	const std::size_t uploadPixelCount = m_packet.TexturePixels.size() - pixelOffset;

	m_packet.TextureUploads.push_back(
	    UiTextureUpload{
	        .Texture = handle,
	        .Width = static_cast<std::uint32_t>(texture.Width),
	        .Height = static_cast<std::uint32_t>(texture.Height),
	        .PixelOffset = static_cast<std::uint32_t>(pixelOffset),
	        .PixelCount = static_cast<std::uint32_t>(uploadPixelCount)});
	return true;
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
		m_packet.Vertices.push_back(
		    UiDrawVertex{.Position = {vertex.pos.x, vertex.pos.y}, .Uv = {vertex.uv.x, vertex.uv.y}, .Color = vertex.col});
	}
	for (ImDrawIdx index : drawList.IdxBuffer)
	{
		m_packet.Indices.push_back(index);
	}
	for (const ImDrawCmd& command : drawList.CmdBuffer)
	{
		const std::uint64_t packedTextureHandle = static_cast<std::uint64_t>(command.GetTexID());
		m_packet.Commands.push_back(
		    UiDrawCommand{
		        .ClipRect = {command.ClipRect.x, command.ClipRect.y, command.ClipRect.z, command.ClipRect.w},
		        .TextureHandle = EditorTextureHandle::Unpack(packedTextureHandle),
		        .ElementCount = command.ElemCount,
		        .IndexOffset = command.IdxOffset,
		        .VertexOffset = static_cast<std::int32_t>(command.VtxOffset),
		        .Kind = command.UserCallback == ImDrawCallback_ResetRenderState ? UiDrawCommandKind::ResetRenderState
		                                                                        : UiDrawCommandKind::Draw});
	}
	m_packet.DrawLists.push_back(packetList);
}
