#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Diagnostics/ScopedLogEvent.h"
#include "Editor/Public/UI.h"
#include "Input/InputSystem.h"
#include "Renderer.h"
#include "RuntimeApplication.h"
#include "Validation/RhiSmokeEditorViewport.h"
#include "Validation/RhiSmokeFrameControl.h"
#include "Validation/RhiSmokeRendererEvidence.h"

#include <string>

namespace
{
	struct EditorSmokeConfig final
	{
		bool Enabled = false;
		bool TraceLogging = false;
		RhiSmokeFrameControlConfig FrameControl;
		RhiSmokeEditorViewportConfig Viewport;
	};

	struct EditorSmokeState final
	{
		bool DiagnosticsLogged = false;
		bool RendererEvidenceLogged = false;
		RhiSmokeFrameControlState FrameControl;
		RhiSmokeEditorViewportState Viewport;
	};

	EditorSmokeConfig LoadConfig() noexcept
	{
		EditorSmokeConfig config{};
		config.Enabled = Environment::GetFlag("SPARKLE_SMOKE_VALIDATE_RHI");
		if (!config.Enabled)
		{
			return config;
		}

		config.TraceLogging = Environment::GetFlag("SPARKLE_SMOKE_TRACE");
		config.FrameControl.Enabled = config.Enabled;
		config.FrameControl.FrameLimit = Environment::GetUInt32("SPARKLE_SMOKE_FRAME_LIMIT", config.FrameControl.FrameLimit);
		config.FrameControl.RestoreFrame = Environment::GetUInt32("SPARKLE_SMOKE_RESTORE_FRAME", config.FrameControl.RestoreFrame);
		config.FrameControl.MaximizeFrame = Environment::GetUInt32("SPARKLE_SMOKE_MAXIMIZE_FRAME", config.FrameControl.MaximizeFrame);
		config.FrameControl.ShaderReloadFrame =
		    Environment::GetUInt32("SPARKLE_SMOKE_SHADER_RELOAD_FRAME", config.FrameControl.ShaderReloadFrame);
		config.FrameControl.LevelSwitching = !Environment::GetFlag("SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING");
		config.FrameControl.LevelSwitchIntervalFrames =
		    Environment::GetUInt32("SPARKLE_SMOKE_LEVEL_SWITCH_INTERVAL_FRAMES", config.FrameControl.LevelSwitchIntervalFrames);
		config.FrameControl.CameraMotion = RhiSmokeCameraMotion::LoadConfig();
		config.Viewport.SceneColorCaptureFrame =
		    Environment::GetUInt32("SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", config.Viewport.SceneColorCaptureFrame);
		Environment::TryGetVariable("SPARKLE_SMOKE_SCENE_COLOR_CAPTURE", config.Viewport.SceneColorCapturePath);
		std::string viewModeValue;
		std::uint32_t viewModeOverride = 0;
		if (Environment::TryGetVariable("SPARKLE_SMOKE_VIEW_MODE", viewModeValue) &&
		    Strings::TryParseNumber(viewModeValue, viewModeOverride) &&
		    viewModeOverride < static_cast<std::uint32_t>(RenderViewMode::Count))
		{
			config.Viewport.HasViewModeOverride = true;
			config.Viewport.ViewModeOverride = static_cast<RenderViewMode>(viewModeOverride);
		}
		return config;
	}

	void ApplyLoggingConfig(const EditorSmokeConfig& config) noexcept
	{
		if (config.Enabled && config.TraceLogging)
		{
			Logging::SetLevel(spdlog::level::trace);
		}
	}

	void LogDiagnosticsCapabilities(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (config.Enabled)
		{
			RhiSmokeRendererEvidence::LogRhiDiagnosticsCapabilities(app, state.DiagnosticsLogged);
		}
	}

	void LogRendererSmokeEvidence(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (!config.Enabled)
		{
			return;
		}

		if (!RhiSmokeRendererEvidence::LogRendererEvidence(
		        app,
		        state.RendererEvidenceLogged,
		        "RHI editor smoke evidence",
		        "RHI editor smoke validation"))
		{
			state.FrameControl.Failed = true;
		}
	}

	void InitializeFrameControl(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		RhiSmokeFrameControl::InitializeLevelSwitching(config.FrameControl, app, state.FrameControl);
	}

	bool TickEditor(RuntimeApplication& app, UI& ui, const EditorSmokeConfig& config, EditorSmokeState& state) noexcept
	{
		switch (app.BeginFrame())
		{
			case RuntimeApplicationFrameResult::Exit:
				return false;
			case RuntimeApplicationFrameResult::SkipRender:
				return true;
			case RuntimeApplicationFrameResult::Ready:
			default:
				break;
		}

		app.UpdateRuntime();
		app.SubmitViewportRenderRequest(ui.GetViewportRenderRequest());

		Renderer& renderer = app.GetRenderer();
		RhiSmokeEditorViewport::ApplyViewModeOverride(config.Viewport, state.Viewport);
		renderer.PrepareHostFrame();
		renderer.RecordHostFrame();
		LogRendererSmokeEvidence(config, app, state);

		const ViewportRenderProducts& viewportProducts = app.GetViewportRenderProducts();
		ui.SetViewportRenderProducts(viewportProducts);
		const ViewportPresentationProduct sceneColorPresentation = renderer.BeginViewportPresentation(RenderOutputFlags::SceneColor);
		ui.SetViewportSceneColorTextureId(sceneColorPresentation.TextureId);
		RhiSmokeEditorViewport::LogEvidence(config.Enabled, viewportProducts, sceneColorPresentation, state.Viewport);
		ui.Update();

		RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();

		constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
		RhiPresentationService& presentationService = renderHardware.GetPresentationService();
		presentationService.BeginPresentRenderPass(editorClearColor);
		ui.Render();
		presentationService.EndPresentRenderPass();

		renderer.EndViewportPresentation(RenderOutputFlags::SceneColor);

		renderer.SubmitHostFrame();
		RhiSmokeEditorViewport::CaptureSceneColorIfRequested(
		    config.Enabled,
		    config.Viewport,
		    app,
		    state.FrameControl.CompletedRenderFrames,
		    state.Viewport,
		    state.FrameControl.Failed);
		app.EndFrame();
		RhiSmokeFrameControl::Advance(config.FrameControl, app, state.FrameControl, "editor");
		return true;
	}
}

int RhiSmokeValidation::RunEditor() noexcept
{
	const EditorSmokeConfig config = LoadConfig();
	RuntimeApplication app(RuntimeApplicationOptions{.EnableRuntimeConsole = false});
	EditorSmokeState state{};
	ApplyLoggingConfig(config);
	app.Initialize();
	LogDiagnosticsCapabilities(config, app, state);
	InitializeFrameControl(config, app, state);
	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");

	{
		SPARKLE_LOG_SCOPE(appLogger, spdlog::level::info, "RHI editor smoke UI scope");
		Renderer& renderer = app.GetRenderer();
		app.GetInputSystem().ClearInputCaptureQuery();
		app.GetInputSystem().BeginInputRoutingFrame(false, false);
		UI ui(
		    EditorHostServices{
		        .RuntimeTimer = app.GetTimer(),
		        .Levels = app.GetLevelManager(),
		        .Scene = app.GetGameScene(),
		        .ImGuiRenderer = renderer.GetImGuiRenderer(),
		        .HostWindow = app.GetWindow(),
		        .Input = app.GetInputSystem()});

		while (TickEditor(app, ui, config, state))
		{
		}
	}

	app.Shutdown();
	SPDLOG_LOGGER_INFO(appLogger, "RHI editor smoke: RuntimeApplication shutdown complete");
	return state.FrameControl.Failed ? 1 : 0;
}
