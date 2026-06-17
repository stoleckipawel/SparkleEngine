#pragma once

class EngineRenderingSettingsSection;

class RenderingSettingsPanel final
{
  public:
	void SetSettings(EngineRenderingSettingsSection* settings) noexcept;
	void RefreshFromRuntimeState() noexcept;
	bool HasPendingRestart() const noexcept;
	void BuildUI(bool disableInteraction, const char* filterText = nullptr);

  private:
	EngineRenderingSettingsSection* m_settings = nullptr;
};
