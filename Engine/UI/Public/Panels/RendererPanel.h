// ============================================================================
// RendererPanel.h
// ----------------------------------------------------------------------------
// Hosts a single right-docked ImGui window with renderer settings sections.
//
#pragma once

#include <memory>
#include <vector>

#include "Framework/UIRendererSection.h"

class RendererPanel final
{
  public:
	explicit RendererPanel(float widthPixels = 384.0f) noexcept;
	~RendererPanel() = default;

	RendererPanel(const RendererPanel&) = delete;
	RendererPanel(RendererPanel&&) = delete;
	RendererPanel& operator=(const RendererPanel&) = delete;
	RendererPanel& operator=(RendererPanel&&) = delete;

	void SetWidth(float widthPixels) noexcept;

	// Replaces an existing section with the same ID (stable ordering), or appends if not present.
	void SetSection(std::unique_ptr<UIRendererSection> section) noexcept;

	bool HasSection(UIRendererSectionId id) const noexcept;
	UIRendererSection& GetSection(UIRendererSectionId id) noexcept;

	// Must be called during an active ImGui frame.
	void BuildUI();

  private:
	std::size_t FindSectionIndex(UIRendererSectionId id) const noexcept;

	float m_widthPixels = 384.0f;
	std::vector<std::unique_ptr<UIRendererSection>> m_sections;
};
