#pragma once

struct ImVec2;
struct ImVec4;

#include <functional>

class LevelSession;
class Window;

class MainMenuBarPanel final
{
public:
	MainMenuBarPanel(LevelSession* levelSession = nullptr, Window* window = nullptr) noexcept;
	~MainMenuBarPanel() = default;

	MainMenuBarPanel(const MainMenuBarPanel&) = delete;
	MainMenuBarPanel(MainMenuBarPanel&&) = delete;
	MainMenuBarPanel& operator=(const MainMenuBarPanel&) = delete;
	MainMenuBarPanel& operator=(MainMenuBarPanel&&) = delete;

	void SetLevelSession(LevelSession* levelSession) noexcept;
	void SetWindow(Window* window) noexcept;
	void SetShaderToolsOpenHandler(std::function<void()> handler);
	void SetMeshToolsOpenHandler(std::function<void()> handler);
	void SetTextureToolsOpenHandler(std::function<void()> handler);
	void SetSettingsOpenHandler(std::function<void()> handler);
	void SetViewportCaptureHandler(std::function<void()> handler);
	void BuildUI() noexcept;
	float GetHeight() const noexcept { return m_heightPixels; }

private:
	void BuildFileMenu() noexcept;
	void BuildWindowsMenu() noexcept;
	void BuildOpenLevelMenu() noexcept;
	void BuildWindowControls() noexcept;
	bool DrawTitleBarButton(
	    const char* id,
	    const ImVec2& size,
	    const ImVec4& baseColor,
	    const ImVec4& hoveredColor,
	    const ImVec4& activeColor) noexcept;
	void DrawMinimizeIcon() const noexcept;
	void DrawMaximizeIcon() const noexcept;
	void DrawCloseIcon() const noexcept;

	LevelSession* m_levelSession = nullptr;
	Window* m_window = nullptr;
	std::function<void()> m_shaderToolsOpenHandler;
	std::function<void()> m_meshToolsOpenHandler;
	std::function<void()> m_textureToolsOpenHandler;
	std::function<void()> m_settingsOpenHandler;
	std::function<void()> m_viewportCaptureHandler;
	float m_heightPixels = 0.0f;
};
