#pragma once

#include <memory>
#include <vector>

#include "Framework/UIRendererSection.h"

class RendererPanel final
{
  public:
	explicit RendererPanel(float widthPixels = 456.0f) noexcept;
	~RendererPanel() = default;

	RendererPanel(const RendererPanel&) = delete;
	RendererPanel(RendererPanel&&) = delete;
	RendererPanel& operator=(const RendererPanel&) = delete;
	RendererPanel& operator=(RendererPanel&&) = delete;

	void SetWidth(float widthPixels) noexcept;
	void SetTopInset(float topInsetPixels) noexcept;

	void SetSection(std::unique_ptr<UIRendererSection> section) noexcept;

	bool HasSection(UIRendererSectionId id) const noexcept;
	UIRendererSection& GetSection(UIRendererSectionId id) noexcept;

	void BuildUI(bool disableInteraction = false);

  private:
	std::size_t FindSectionIndex(UIRendererSectionId id) const noexcept;

	float m_widthPixels = 456.0f;
	float m_topInsetPixels = 0.0f;
	std::vector<std::unique_ptr<UIRendererSection>> m_sections;
};
