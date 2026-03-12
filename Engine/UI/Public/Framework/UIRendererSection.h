// ============================================================================
// UIRendererSection.h
// ----------------------------------------------------------------------------
// Base interface and IDs for sections within the renderer settings panel.
//
#pragma once

#include <cstdint>

// ============================================================================
// Section Identifiers
// ============================================================================

/// Stable IDs for sections rendered inside the renderer panel.
/// Used for replacement, ordering, and typed retrieval.
enum class UIRendererSectionId : std::uint8_t
{
	Stats = 0,  ///< Performance statistics (FPS, frame time)
	ViewMode,   ///< Render view mode selection (lit, wireframe, etc.)
	Time,       ///< Time control (speed, pause)
	Scene,      ///< Scene configuration (primitives, etc.)
	Camera,     ///< Camera settings (FOV, position)
	Count       ///< Number of section IDs (for array sizing)
};

// ============================================================================
// UIRendererSection Interface
// ============================================================================

/// Base interface for a renderable section inside the renderer panel.
/// Sections are owned by the panel and are expected to be long-lived.
class UIRendererSection
{
  public:
	virtual ~UIRendererSection() = default;

	/// Returns the stable ID for this section type.
	virtual UIRendererSectionId GetId() const noexcept = 0;

	/// Returns the display title for the collapsing header.
	virtual const char* GetTitle() const noexcept = 0;

	/// Called each frame to build ImGui content for this section.
	virtual void BuildUI() = 0;
};
