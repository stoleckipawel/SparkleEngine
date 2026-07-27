#include "PCH.h"
#include "UI/UiRenderPacketPlayer.h"

#include "Editor/EditorTextureRegistry.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>

struct UiRenderPacketPlayer::PlaybackStorage final
{
	struct Texture final
	{
		EditorTextureHandle Handle;
		std::unique_ptr<ImTextureData> Data;
		bool PendingRelease = false;
	};

	std::vector<std::unique_ptr<ImDrawList>> DrawLists;
	std::vector<Texture> Textures;
	ImVector<ImTextureData*> TextureUpdates;
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
	const bool hasPendingRelease = std::any_of(
	    m_storage->Textures.begin(),
	    m_storage->Textures.end(),
	    [](const PlaybackStorage::Texture& texture)
	    {
		    return texture.PendingRelease;
	    });
	if (!packet.HasDrawData() &&
	    packet.TextureUploads.empty() &&
	    packet.TextureReleases.empty() &&
	    !hasPendingRelease)
	{
		return;
	}

	ApplyTextureUpdates(packet);
	PrepareDrawLists(packet.DrawLists.size());
	for (std::size_t drawListIndex = 0; drawListIndex < packet.DrawLists.size(); ++drawListIndex)
	{
		CopyDrawList(packet, packet.DrawLists[drawListIndex], drawListIndex, textures);
	}

	PrepareDrawData(packet);
	renderer.RenderDrawData(&m_storage->DrawData);
	RetireReleasedTextures();
}

void UiRenderPacketPlayer::Shutdown(RhiImGuiRenderer& renderer) noexcept
{
	for (PlaybackStorage::Texture& texture : m_storage->Textures)
	{
		renderer.ReleaseTexture(*texture.Data);
	}

	m_storage->Textures.clear();
	m_storage->TextureUpdates.resize(0);
}

void UiRenderPacketPlayer::ApplyTextureUpdates(const UiRenderPacket& packet)
{
	m_storage->TextureUpdates.resize(0);

	for (const UiTextureUpload& upload : packet.TextureUploads)
	{
		ApplyTextureUpload(packet, upload);
	}

	for (EditorTextureHandle handle : packet.TextureReleases)
	{
		QueueTextureRelease(handle);
	}

	for (PlaybackStorage::Texture& texture : m_storage->Textures)
	{
		if (!texture.PendingRelease)
		{
			continue;
		}

		++texture.Data->UnusedFrames;
		if (std::find(
		        m_storage->TextureUpdates.begin(),
		        m_storage->TextureUpdates.end(),
		        texture.Data.get()) == m_storage->TextureUpdates.end())
		{
			m_storage->TextureUpdates.push_back(texture.Data.get());
		}
	}
}

void UiRenderPacketPlayer::ApplyTextureUpload(
    const UiRenderPacket& packet,
    const UiTextureUpload& upload)
{
	const std::size_t expectedPixelCount =
	    static_cast<std::size_t>(upload.Width) *
	    static_cast<std::size_t>(upload.Height) * 4;
	if (!upload.Texture.IsImGuiTexture() ||
	    expectedPixelCount != upload.PixelCount ||
	    upload.PixelOffset > packet.TexturePixels.size() ||
	    upload.PixelCount > packet.TexturePixels.size() - upload.PixelOffset)
	{
		return;
	}

	ImTextureData* texture = FindTexture(upload.Texture);
	const bool isNewTexture = texture == nullptr;
	if (isNewTexture)
	{
		auto data = std::make_unique<ImTextureData>();
		data->UniqueID = static_cast<int>(upload.Texture.Generation);
		data->Create(
		    ImTextureFormat_RGBA32,
		    static_cast<int>(upload.Width),
		    static_cast<int>(upload.Height));
		texture = data.get();
		m_storage->Textures.push_back(PlaybackStorage::Texture{
		    .Handle = upload.Texture,
		    .Data = std::move(data)});
	}
	else if (texture->WantDestroyNextFrame ||
	         texture->Width != static_cast<int>(upload.Width) ||
	         texture->Height != static_cast<int>(upload.Height))
	{
		return;
	}

	const std::byte* source =
	    packet.TexturePixels.data() + upload.PixelOffset;
	std::memcpy(texture->Pixels, source, upload.PixelCount);
	if (!isNewTexture)
	{
		texture->UpdateRect = ImTextureRect{
		    .x = 0,
		    .y = 0,
		    .w = static_cast<unsigned short>(upload.Width),
		    .h = static_cast<unsigned short>(upload.Height)};
		texture->SetStatus(ImTextureStatus_WantUpdates);
	}

	m_storage->TextureUpdates.push_back(texture);
}

void UiRenderPacketPlayer::QueueTextureRelease(
    EditorTextureHandle handle) noexcept
{
	ImTextureData* texture = FindTexture(handle);
	if (texture == nullptr)
	{
		return;
	}

	for (PlaybackStorage::Texture& storedTexture : m_storage->Textures)
	{
		if (storedTexture.Data.get() != texture)
		{
			continue;
		}

		storedTexture.PendingRelease = true;
		texture->WantDestroyNextFrame = true;
		texture->SetStatus(ImTextureStatus_WantDestroy);
		return;
	}
}

ImTextureData* UiRenderPacketPlayer::FindTexture(
    EditorTextureHandle handle) const noexcept
{
	for (const PlaybackStorage::Texture& texture : m_storage->Textures)
	{
		if (texture.Handle == handle)
		{
			return texture.Data.get();
		}
	}

	return nullptr;
}

void UiRenderPacketPlayer::RetireReleasedTextures() noexcept
{
	std::erase_if(
	    m_storage->Textures,
	    [](const PlaybackStorage::Texture& texture)
	    {
		    return texture.PendingRelease &&
		           texture.Data->BackendUserData == nullptr;
	    });

	m_storage->TextureUpdates.resize(0);
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
		if (ImTextureData* texture = FindTexture(source.TextureHandle))
		{
			command.TexRef = texture->GetTexRef();
		}
		else
		{
			command.TexRef = ImTextureRef(
			    static_cast<ImTextureID>(
			        textures.Resolve(source.TextureHandle)));
		}
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
	drawData.Textures = m_storage->TextureUpdates.Size != 0
	                        ? &m_storage->TextureUpdates
	                        : nullptr;
}
