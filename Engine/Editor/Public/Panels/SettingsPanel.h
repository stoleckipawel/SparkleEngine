#pragma once

#include <memory>
#include <functional>
#include <string>

class EngineRenderingSettingsSection;
class RenderingSettingsPanel;

class SettingsPanel final
{
public:
	SettingsPanel();
	~SettingsPanel();

	void SetOpen(bool open) noexcept;
	bool IsOpen() const noexcept { return m_isOpen; }
	void SetRenderingSettings(EngineRenderingSettingsSection* renderingSettings) noexcept;
	void SetRestartHandler(std::function<void()> restartHandler);
	void BuildUI(bool disableInteraction);

private:
	void DrawToolbar();
	bool HasPendingRestart() const noexcept;

	EngineRenderingSettingsSection* m_renderingSettings = nullptr;
	std::unique_ptr<RenderingSettingsPanel> m_renderingPanel;
	std::function<void()> m_restartHandler;
	std::string m_filterText;
	bool m_isOpen = false;
	bool m_refreshFromRuntimeOnNextOpen = true;
};
