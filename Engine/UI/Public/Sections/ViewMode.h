#pragma once

#include <cstdint>

#include "Framework/UIRendererSection.h"

class ViewMode final : public UIRendererSection
{
  public:
	enum class Type : std::uint8_t
	{
		Lit = 0,
		GBufferDiffuse,
		GBufferNormal,
		GBufferRoughness,
		GBufferMetallic,
		GBufferEmissive,
		GBufferAmbientOcclusion,
		GBufferSubsurfaceColor,
		GBufferSubsurfaceStrength,
		DirectDiffuse,
		DirectSpecular,
		DirectSubsurface,
		IndirectDiffuse,
		IndirectSpecular,
		Count
	};

	ViewMode() noexcept = default;
	~ViewMode() = default;

	ViewMode(const ViewMode&) = delete;
	ViewMode(ViewMode&&) = delete;
	ViewMode& operator=(const ViewMode&) = delete;
	ViewMode& operator=(ViewMode&&) = delete;

	Type Get() const noexcept { return m_mode; }
	void Set(Type mode) noexcept { m_mode = mode; }

	UIRendererSectionId GetId() const noexcept override { return UIRendererSectionId::ViewMode; }
	const char* GetTitle() const noexcept override { return "View Modes"; }

	void BuildUI() override;

  private:
	Type m_mode = Type::Lit;
};
