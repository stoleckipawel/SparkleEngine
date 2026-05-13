#pragma once

#include "Renderer/Public/Textures/TextureDiagnostics.h"

#include <array>
#include <functional>
#include <string>

class UsedTexturesPanel final
{
  public:
	using DiagnosticsProvider = std::function<TextureDiagnosticsSnapshot()>;

	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }
	void SetDiagnosticsProvider(DiagnosticsProvider provider);
	void BuildUI(bool disableInteraction);

  private:
	void RefreshSnapshot();
	void DrawToolbar();
	void DrawTextureTable(bool disableInteraction);
	void DrawSelectedTextureInspector(bool disableInteraction);
	void DrawPreview(const TextureDiagnosticsRow& row) const;
	void DrawSelectedTextureDetails(const TextureDiagnosticsRow& row) const;
	const TextureDiagnosticsRow* GetSelectedRow() const noexcept;
	bool MatchesFilter(const TextureDiagnosticsRow& row) const noexcept;

	DiagnosticsProvider m_diagnosticsProvider;
	TextureDiagnosticsSnapshot m_snapshot;
	std::array<char, 160> m_filterBuffer{};
	std::string m_selectedKey;
	bool m_isOpen = false;
};