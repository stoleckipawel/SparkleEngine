#pragma once

#include "Application.h"
#include "RuntimeApplication.h"

#include <memory>

struct EditorApplicationOptions final
{
	RuntimeApplicationOptions RuntimeOptions;
};

class RuntimeApplication;
class ShaderRecookCoordinator;
class UI;

class SPARKLE_APPLICATION_API EditorApplication final : public Application
{
  public:
	EditorApplication();
	explicit EditorApplication(EditorApplicationOptions options) noexcept;
	~EditorApplication();

	EditorApplication(const EditorApplication&) = delete;
	EditorApplication& operator=(const EditorApplication&) = delete;
	EditorApplication(EditorApplication&&) = delete;
	EditorApplication& operator=(EditorApplication&&) = delete;

	void Initialize() override;
	bool Tick() override;
	void Shutdown() override;

  private:
	std::unique_ptr<RuntimeApplication> m_runtimeApplication;
	std::unique_ptr<UI> m_ui;
	std::unique_ptr<ShaderRecookCoordinator> m_shaderRecookCoordinator;
	EditorApplicationOptions m_options;
	bool m_isEditorSessionActive = false;
};
