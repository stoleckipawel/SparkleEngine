#pragma once

#include "Core/Public/Console/CVarControl.h"

#include "EditorAPI.h"
#include "Renderer/Public/Diagnostics/MeshPreviewGeometry.h"
#include "Renderer/Public/Diagnostics/RendererMemoryDiagnostics.h"
#include "Renderer/Public/UI/UiRenderPacket.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Renderer/Public/Meshes/MeshDiagnostics.h"
#include "Renderer/Public/Resources/Textures/TextureDiagnostics.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Panels/ViewportOutputAction.h"
#include "Viewport/ViewportOverlay.h"
#include "GameFramework/Public/Rendering/RenderViewCameraData.h"
#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "GameFramework/Public/World/WorldChange.h"
#include "GameFramework/Public/World/WorldEditCommand.h"
#include "GameFramework/Public/World/WorldMaterialVariantView.h"
#include "GameFramework/Public/World/WorldReadView.h"

#include <cstdint>
#include <functional>
#include <memory>

class Timer;
class EditorConsoleSystem;
class InputSystem;
class LevelSession;
class Window;

struct EditorHostServices final
{
	Timer& RuntimeTimer;
	LevelSession* Levels = nullptr;
	std::function<WorldReadView()> AcquireWorldReadView;
	std::function<WorldChangeBatch(const WorldChangeCursor&)> ReadWorldChanges;
	std::function<bool(WorldChangeCursor&, WorldSequence)> AcknowledgeWorldChanges;
	std::function<std::uint64_t()> WorldGeneration;
	std::function<WorldMaterialVariantView()> MaterialVariants;
	std::function<WorldEditResult(WorldEditCommand, std::uint64_t)> SubmitWorldEdit;
	EngineRenderingSettingsState RenderingSettings;
	std::function<void(EngineRenderingSettingsState)> SubmitRenderingSettings;
	std::function<EngineRenderingSettingsState()> CaptureRenderingSettings;
	CVarControlExecutor ConsoleVariables;
	Window& HostWindow;
	InputSystem& Input;
};

struct EditorDiagnosticsProviders final
{
	std::function<std::uint64_t()> ShaderGeneration;
	std::function<MeshDiagnosticsSnapshot()> MeshDiagnostics;
	std::function<TextureDiagnosticsSnapshot()> TextureDiagnostics;
	std::function<RendererMemoryDiagnosticsSnapshot()> MemoryDiagnostics;
	std::function<MeshPreviewGeometry(std::uintptr_t)> MeshPreview;
};

class SPARKLE_EDITOR_API UI final
{
public:
	explicit UI(EditorHostServices hostServices);

	~UI() noexcept;

	UI(const UI&) = delete;
	UI& operator=(const UI&) = delete;
	UI(UI&&) = delete;
	UI& operator=(UI&&) = delete;

	const ViewportRenderRequest& GetViewportRenderRequest() const noexcept;
	RenderViewCameraData UpdateViewportCamera(const CameraInputIntent& intent, float deltaSeconds) noexcept;
	// Install, replace or clear the viewport overlay after UI construction on the Editor thread.
	void SetViewportOverlay(std::unique_ptr<ViewportOverlay> overlay) noexcept;
	void SetViewportRenderProducts(const ViewportRenderProducts& products) noexcept;
	void SetViewportFinalColorTexture(UiTextureHandle texture) noexcept;
	void SetDiagnosticsProviders(EditorDiagnosticsProviders providers);
	RendererMemoryDiagnosticsSnapshot CaptureMemoryDiagnostics() const;
	EditorConsoleSystem* GetEditorConsoleSystem() noexcept;
	bool ConsumeShaderReloadRequest() noexcept;
	bool ConsumeShaderRecookRequest() noexcept;
	ViewportOutputAction ConsumeViewportOutputAction() noexcept;
	UiRenderPacket ConsumeRenderPacket();

	void Update();

private:
	class Implementation;
	std::unique_ptr<Implementation> m_implementation;
};
