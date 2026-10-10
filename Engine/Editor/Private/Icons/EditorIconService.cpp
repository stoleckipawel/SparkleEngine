#include "PCH.h"
#include "Editor/Public/Icons/EditorIconService.h"

#include "Editor/Public/Icons/EditorIconAsset.h"

#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <thread>
#include <vector>

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_editorIconLogger, "EditorIcons");

class EditorIconService::Implementation final
{
public:
	Implementation() :
	    m_context(ImGui::GetCurrentContext()),
	    m_atlas(m_context != nullptr ? ImGui::GetIO().Fonts : nullptr),
	    m_thread(std::this_thread::get_id())
	{
		if (m_atlas == nullptr)
		{
			Diagnostics::Fatal(g_editorIconLogger, __FILE__, __LINE__, "EditorIconService requires an initialized UI context");
		}
	}

	~Implementation() noexcept
	{
		ValidateOwner();
		for (const RegisteredIcon& icon : m_icons)
		{
			m_atlas->RemoveCustomRect(icon.Rectangle);
		}
	}

	bool DrawButton(const EditorIconAsset& asset, const char* id, float imageSize) noexcept;

private:
	struct RegisteredIcon final
	{
		const EditorIconAsset* Asset;
		ImFontAtlasRectId Rectangle;
	};

	void ValidateOwner() const noexcept
	{
		if (std::this_thread::get_id() != m_thread || ImGui::GetCurrentContext() != m_context || ImGui::GetIO().Fonts != m_atlas)
		{
			Diagnostics::Fatal(
			    g_editorIconLogger,
			    __FILE__,
			    __LINE__,
			    "EditorIconService used outside its owning thread/context/atlas lifetime");
		}
	}

	ImFontAtlasRect Resolve(const EditorIconAsset& asset) noexcept;

	ImGuiContext* const m_context;
	ImFontAtlas* const m_atlas;
	const std::thread::id m_thread;
	std::vector<RegisteredIcon> m_icons;
};

ImFontAtlasRect EditorIconService::Implementation::Resolve(const EditorIconAsset& asset) noexcept
{
	ValidateOwner();
	auto found = std::ranges::find(m_icons, &asset, &RegisteredIcon::Asset);
	ImFontAtlasRect rectangle;
	if (found != m_icons.end() && m_atlas->GetCustomRect(found->Rectangle, &rectangle))
	{
		return rectangle;
	}

	if (asset.Extent < 1 || asset.Extent > 512 || asset.Pixels.size() != static_cast<std::size_t>(asset.Extent) * asset.Extent * 4)
	{
		Diagnostics::Fatal(g_editorIconLogger, __FILE__, __LINE__, "Editor icon requires square RGBA8 pixels and extent in [1,512]");
	}
	const ImFontAtlasRectId rectangleId = m_atlas->AddCustomRect(asset.Extent, asset.Extent, &rectangle);
	if (rectangleId == ImFontAtlasRectId_Invalid || m_atlas->TexData->Format != ImTextureFormat_RGBA32)
	{
		Diagnostics::Fatal(g_editorIconLogger, __FILE__, __LINE__, "Editor icon could not register an RGBA32 atlas rectangle");
	}
	m_atlas->TexPixelsUseColors = true;
	for (int row = 0; row < asset.Extent; ++row)
	{
		std::memcpy(
		    m_atlas->TexData->GetPixelsAt(rectangle.x, rectangle.y + row),
		    asset.Pixels.data() + static_cast<std::size_t>(row) * asset.Extent * 4,
		    static_cast<std::size_t>(asset.Extent) * 4);
	}
	if (found != m_icons.end())
	{
		found->Rectangle = rectangleId;
	}
	else
	{
		m_icons.push_back({&asset, rectangleId});
	}
	// AddCustomRect queues the upload; the existing render packet transfers pixels.
	return rectangle;
}

bool EditorIconService::Implementation::DrawButton(const EditorIconAsset& asset, const char* id, float imageSize) noexcept
{
	// Resolve every draw: packing/growth can change both coordinates and TexRef.
	const ImFontAtlasRect rectangle = Resolve(asset);
	if (imageSize <= 0.0f)
	{
		imageSize = ImGui::GetFontSize();
	}
	const float padding = ImGui::GetStyle().FramePadding.y;
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(padding, padding));
	const bool pressed = ImGui::ImageButton(id, m_atlas->TexRef, ImVec2(imageSize, imageSize), rectangle.uv0, rectangle.uv1);
	ImGui::PopStyleVar();
	return pressed;
}

EditorIconService::EditorIconService() :
    m_implementation(std::make_unique<Implementation>())
{
}

EditorIconService::~EditorIconService() noexcept = default;

bool EditorIconService::DrawButton(const EditorIconAsset& asset, const char* id, float imageSize) noexcept
{
	return m_implementation->DrawButton(asset, id, imageSize);
}
