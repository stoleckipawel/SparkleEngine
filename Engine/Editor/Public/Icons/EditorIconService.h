#pragma once

#include "Editor/Public/EditorAPI.h"

#include <memory>

struct EditorIconAsset;

// UI-context resource owner. Construct after typography, destroy after clients
// and before ImGui teardown; calls stay on the creating Editor thread/context.
// Assets are borrowed for this lifetime. No native texture/atlas handle escapes.
class SPARKLE_EDITOR_API EditorIconService final
{
public:
	EditorIconService();
	~EditorIconService() noexcept;
	EditorIconService(const EditorIconService&) = delete;
	EditorIconService& operator=(const EditorIconService&) = delete;
	EditorIconService(EditorIconService&&) = delete;
	EditorIconService& operator=(EditorIconService&&) = delete;

	// Square current-font-sized image with ordinary frame-height hit area. The
	// caller owns ID, layout, enabled state and tooltip; returns actual activation.
	bool DrawButton(const EditorIconAsset& asset, const char* id) noexcept;

private:
	class Implementation;
	std::unique_ptr<Implementation> m_implementation;
};
