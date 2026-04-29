#pragma once

#include "../EditorAPI.h"
#include "../../../Renderer/Public/Viewport/ViewportContracts.h"

#include <cstdint>

class SPARKLE_EDITOR_API ViewportPanel final
{
  public:
	ViewportPanel(float leftInsetPixels = 320.0f, float rightInsetPixels = 456.0f) noexcept;
	~ViewportPanel() = default;

	ViewportPanel(const ViewportPanel&) = delete;
	ViewportPanel(ViewportPanel&&) = delete;
	ViewportPanel& operator=(const ViewportPanel&) = delete;
	ViewportPanel& operator=(ViewportPanel&&) = delete;

	void SetTopInset(float topInsetPixels) noexcept;
	void SetBottomInset(float bottomInsetPixels) noexcept;
	void SetSideInsets(float leftInsetPixels, float rightInsetPixels) noexcept;
	void SetRequestedExtent(RenderViewportExtent extent) noexcept;
	void SetRenderProducts(const ViewportRenderProducts& renderProducts) noexcept;
	void SetSceneColorTextureId(std::uint64_t textureId) noexcept;
	const ViewportRenderRequest& GetRenderRequest() const noexcept;
	bool WantsGameplayInput() const noexcept { return m_wantsGameplayInput; }
	void BuildUI(bool disableInteraction = false);

  private:
	void UpdateRequestedExtent(float availableWidth, float availableHeight) noexcept;
	void BuildEmptyState() noexcept;

	ViewportRenderRequest m_renderRequest = {};
	ViewportRenderProducts m_renderProducts = {};
	float m_topInsetPixels = 0.0f;
	float m_bottomInsetPixels = 0.0f;
	float m_leftInsetPixels = 320.0f;
	float m_rightInsetPixels = 456.0f;
	std::uint64_t m_sceneColorTextureId = 0;
	bool m_hasInputFocus = false;
	bool m_wantsGameplayInput = false;
};