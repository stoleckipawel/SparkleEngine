#pragma once

#include "../EditorAPI.h"
#include "../../../Renderer/Public/Viewport/ViewportContracts.h"
#include "Input/Dispatch/InputLayer.h"

#include <cstdint>

class SPARKLE_EDITOR_API ViewportPanel final
{
  public:
	ViewportPanel(float leftInsetPixels = 320.0f, float rightInsetPixels = 456.0f) noexcept;
	~ViewportPanel() noexcept;

	ViewportPanel(const ViewportPanel&) = delete;
	ViewportPanel(ViewportPanel&&) = delete;
	ViewportPanel& operator=(const ViewportPanel&) = delete;
	ViewportPanel& operator=(ViewportPanel&&) = delete;

	void SetTopInset(float topInsetPixels) noexcept;
	void SetBottomInset(float bottomInsetPixels) noexcept;
	void SetSideInsets(float leftInsetPixels, float rightInsetPixels) noexcept;
	void SetRequestedExtent(RenderViewportExtent extent) noexcept;
	void SetRenderProducts(const ViewportRenderProducts& renderProducts) noexcept;
	void SetSceneColorTexture(EditorTextureHandle texture) noexcept;
	const ViewportRenderRequest& GetRenderRequest() const noexcept;
	InputLayer GetTargetInputLayer() const noexcept { return InputLayer::Gameplay; }
	bool GetInputBounds(float& left, float& top, float& right, float& bottom) const noexcept;
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
	EditorTextureHandle m_sceneColorTexture;
	float m_inputLeft = 0.0f;
	float m_inputTop = 0.0f;
	float m_inputRight = 0.0f;
	float m_inputBottom = 0.0f;
	bool m_hasInputBounds = false;
};
