#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Diagnostics/ScopedLogEvent.h"
#include "Editor/Public/UI.h"
#include "Input/InputSystem.h"
#include "Renderer.h"
#include "RuntimeApplication.h"
#include "Validation/RhiSmokeFrameControl.h"
#include "Validation/RhiSmokeRenderViewModeNames.h"
#include "Validation/RhiSmokeSession.h"
#include "Validation/RhiSmokeViewportCapture.h"

#include <string>

namespace
{
	struct EditorSmokeConfig final
	{
		RhiSmokeSessionConfig Session;
		RhiSmokeViewportCaptureConfig Viewport;
	};

	struct EditorSmokeState final
	{
		RhiSmokeSessionState Session;
		RhiSmokeViewportCaptureState Viewport;
	};

	EditorSmokeConfig LoadConfig() noexcept
	{
		EditorSmokeConfig config{};
		config.Session = RhiSmokeSession::LoadConfig();
		if (!config.Session.Enabled)
		{
			return config;
		}

		config.Viewport.SceneColorCaptureFrame =
		    Environment::GetUInt32("SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", config.Viewport.SceneColorCaptureFrame);
		Environment::TryGetVariable("SPARKLE_SMOKE_SCENE_COLOR_CAPTURE", config.Viewport.SceneColorCapturePath);
		Environment::TryGetVariable("SPARKLE_SMOKE_METADATA_PATH", config.Viewport.MetadataPath);
		Environment::TryGetVariable("SPARKLE_SMOKE_TIMING_CSV", config.Viewport.TimingCsvPath);
		Environment::TryGetVariable("SPARKLE_SMOKE_CAPTURE_PURPOSE", config.Viewport.CapturePurpose);
		Environment::TryGetVariable("SPARKLE_SMOKE_CAPTURE_LABEL", config.Viewport.CaptureLabel);
		std::string viewModeValue;
		std::string viewModeName;
		std::string ptlasCapturePreset;
		std::uint32_t viewModeOverride = 0;
		if (Environment::TryGetVariable("SPARKLE_SMOKE_VIEW_MODE_NAME", viewModeName) &&
		    RhiSmokeRenderViewModeNames::TryParse(viewModeName, config.Viewport.ViewModeOverride))
		{
			config.Viewport.HasViewModeOverride = true;
		}
		else if (Environment::TryGetVariable("SPARKLE_SMOKE_PTLAS_CAPTURE_PRESET", ptlasCapturePreset) &&
		         RhiSmokeRenderViewModeNames::TryParse(ptlasCapturePreset, config.Viewport.ViewModeOverride))
		{
			config.Viewport.HasViewModeOverride = true;
		}
		else if (Environment::TryGetVariable("SPARKLE_SMOKE_VIEW_MODE", viewModeValue) &&
		    Strings::TryParseNumber(viewModeValue, viewModeOverride) &&
		    viewModeOverride < static_cast<std::uint32_t>(RenderViewMode::Count))
		{
			config.Viewport.HasViewModeOverride = true;
			config.Viewport.ViewModeOverride = static_cast<RenderViewMode>(viewModeOverride);
		}
		return config;
	}

	void LogDiagnosticsCapabilities(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (config.Session.Enabled)
		{
			RhiSmokeSession::LogDiagnosticsCapabilities(config.Session, app, state.Session);
		}
	}

	void LogRendererSmokeEvidence(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (!config.Session.Enabled)
		{
			return;
		}

		RhiSmokeSession::LogRendererEvidence(config.Session, app, state.Session, "RHI editor smoke validation");
	}

	void InitializeFrameControl(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		RhiSmokeSession::InitializeFrameControl(config.Session, app, state.Session);
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
		RhiSmokeViewportCapture::ApplyViewModeOverride(config.Viewport, state.Viewport);
		renderer.PrepareHostFrame();
		renderer.RecordHostFrame();
		LogRendererSmokeEvidence(config, app, state);

		const ViewportRenderProducts& viewportProducts = app.GetViewportRenderProducts();
		ui.SetViewportRenderProducts(viewportProducts);
		const ViewportPresentationProduct sceneColorPresentation = renderer.BeginViewportPresentation(RenderOutputFlags::SceneColor);
		ui.SetViewportSceneColorTextureId(sceneColorPresentation.TextureId);
		RhiSmokeViewportCapture::LogEvidence(config.Session.Enabled, viewportProducts, sceneColorPresentation, state.Viewport);
		ui.Update();

		RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();

		constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
		RhiPresentationService& presentationService = renderHardware.GetPresentationService();
		presentationService.BeginPresentRenderPass(editorClearColor);
		ui.Render();
		presentationService.EndPresentRenderPass();

		renderer.EndViewportPresentation(RenderOutputFlags::SceneColor);

		renderer.SubmitHostFrame();
		RhiSmokeViewportCapture::CaptureSceneColorIfRequested(
		    config.Session.Enabled,
		    config.Viewport,
		    app,
		    state.Session.FrameControl.CompletedRenderFrames,
		    state.Viewport,
		    state.Session.FrameControl.Failed);
		RhiSmokeFrameControl::Advance(config.Session.FrameControl, app, state.Session.FrameControl, "editor");
		return true;
	}
}

namespace
{
	int RunEditorValidation(EditorApplicationOptions options) noexcept
	{
		const EditorSmokeConfig config = LoadConfig();
		EditorSmokeState state{};
		RuntimeApplicationOptions runtimeOptions = std::move(options.RuntimeOptions);
		runtimeOptions.EnableRuntimeConsole = false;
		RuntimeApplication app(std::move(runtimeOptions));
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
			for (;;)
			{
				if (!TickEditor(app, ui, config, state))
				{
					break;
				}
			}
		}

		app.Shutdown();
		return state.Session.FrameControl.Failed ? 1 : 0;
	}
}

int RhiSmokeValidation::RunEditor() noexcept
{
	return RunEditorValidation(EditorApplicationOptions{});
}

int RhiSmokeValidation::RunEditor(EditorApplicationOptions options) noexcept
{
	return RunEditorValidation(std::move(options));
}
