#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Diagnostics/ScopedLogEvent.h"
#include "Editor/Public/UI.h"
#include "Input/InputSystem.h"
#include "Level/Level.h"
#include "Level/LevelManager.h"
#include "Platform/Public/Window/Window.h"
#include "Renderer.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RHI/Public/Core/RhiBackendSelection.h"
#include "RuntimeApplication.h"
#include "Validation/RhiSmokeCameraMotion.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
	struct EditorSmokeConfig final
	{
		bool Enabled = false;
		bool TraceLogging = false;
		std::uint32_t FrameLimit = 120;
		std::uint32_t RestoreFrame = 10;
		std::uint32_t MaximizeFrame = 20;
		std::uint32_t ShaderReloadFrame = 0;
		bool LevelSwitching = true;
		std::uint32_t LevelSwitchIntervalFrames = 15;
		std::uint32_t SceneColorCaptureFrame = 20;
		std::string SceneColorCapturePath;
		bool HasViewModeOverride = false;
		RenderViewMode ViewModeOverride = RenderViewMode::Lit;
		RhiSmokeCameraMotionConfig CameraMotion;
	};

	struct EditorSmokeState final
	{
		std::uint32_t CompletedRenderFrames = 0;
		bool DiagnosticsLogged = false;
		bool RendererEvidenceLogged = false;
		bool EditorViewportEvidenceLogged = false;
		bool LevelSwitchingInitialized = false;
		bool LevelSwitchingFinished = false;
		bool Failed = false;
		std::uint32_t LastLevelSwitchFrame = 0;
		std::uint32_t CompletedLevelSwitches = 0;
		std::vector<std::string> LevelSwitchOrder;
		std::string PendingLevelName;
		bool SceneColorCaptured = false;
		bool ViewModeOverrideLogged = false;
		RhiSmokeCameraMotionState CameraMotion;
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
		config.FrameLimit = Environment::GetUInt32("SPARKLE_SMOKE_FRAME_LIMIT", config.FrameLimit);
		config.RestoreFrame = Environment::GetUInt32("SPARKLE_SMOKE_RESTORE_FRAME", config.RestoreFrame);
		config.MaximizeFrame = Environment::GetUInt32("SPARKLE_SMOKE_MAXIMIZE_FRAME", config.MaximizeFrame);
		config.ShaderReloadFrame = Environment::GetUInt32("SPARKLE_SMOKE_SHADER_RELOAD_FRAME", config.ShaderReloadFrame);
		config.LevelSwitching = !Environment::GetFlag("SPARKLE_SMOKE_SKIP_LEVEL_SWITCHING");
		config.LevelSwitchIntervalFrames = Environment::GetUInt32(
		    "SPARKLE_SMOKE_LEVEL_SWITCH_INTERVAL_FRAMES",
		    config.LevelSwitchIntervalFrames);
		config.CameraMotion = RhiSmokeCameraMotion::LoadConfig();
		config.SceneColorCaptureFrame = Environment::GetUInt32("SPARKLE_SMOKE_SCENE_COLOR_CAPTURE_FRAME", config.SceneColorCaptureFrame);
		Environment::TryGetVariable("SPARKLE_SMOKE_SCENE_COLOR_CAPTURE", config.SceneColorCapturePath);
		std::string viewModeValue;
		std::uint32_t viewModeOverride = 0;
		if (Environment::TryGetVariable("SPARKLE_SMOKE_VIEW_MODE", viewModeValue) &&
		    Strings::TryParseNumber(viewModeValue, viewModeOverride) &&
		    viewModeOverride < static_cast<std::uint32_t>(RenderViewMode::Count))
		{
			config.HasViewModeOverride = true;
			config.ViewModeOverride = static_cast<RenderViewMode>(viewModeOverride);
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

	const char* RenderViewModeName(RenderViewMode viewMode) noexcept
	{
		switch (viewMode)
		{
			case RenderViewMode::Lit:
				return "Lit";
			case RenderViewMode::Wireframe:
				return "Wireframe";
			case RenderViewMode::GBufferDiffuse:
				return "GBufferDiffuse";
			case RenderViewMode::GBufferNormal:
				return "GBufferNormal";
			case RenderViewMode::GBufferRoughness:
				return "GBufferRoughness";
			case RenderViewMode::GBufferMetallic:
				return "GBufferMetallic";
			case RenderViewMode::GBufferEmissive:
				return "GBufferEmissive";
			case RenderViewMode::GBufferAmbientOcclusion:
				return "GBufferAmbientOcclusion";
			case RenderViewMode::GBufferSubsurfaceColor:
				return "GBufferSubsurfaceColor";
			case RenderViewMode::GBufferSubsurfaceStrength:
				return "GBufferSubsurfaceStrength";
			case RenderViewMode::DirectDiffuse:
				return "DirectDiffuse";
			case RenderViewMode::DirectSpecular:
				return "DirectSpecular";
			case RenderViewMode::DirectSubsurface:
				return "DirectSubsurface";
			case RenderViewMode::IndirectDiffuse:
				return "IndirectDiffuse";
			case RenderViewMode::IndirectSpecular:
				return "IndirectSpecular";
			case RenderViewMode::IndirectSubsurface:
				return "IndirectSubsurface";
			case RenderViewMode::InstanceGroups:
				return "InstanceGroups";
			case RenderViewMode::Count:
				break;
		}

		return "Unknown";
	}

	std::string GetActiveLevelName(const RuntimeApplication& app)
	{
		const LevelManager* levelManager = app.GetLevelManager();
		if (levelManager == nullptr)
		{
			return {};
		}

		const LevelAsset* activeLevel = levelManager->GetActiveLevel();
		return activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string();
	}

	void LogDiagnosticsCapabilities(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (!config.Enabled || state.DiagnosticsLogged)
		{
			return;
		}

		static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
		if (appLogger == nullptr)
		{
			return;
		}

		const RenderHardwareInterface& renderHardware = app.GetRenderer().GetRenderHardwareInterface();
		const RhiDiagnosticsCapabilities capabilities = renderHardware.GetDiagnosticsService().GetDiagnostics().GetCapabilities();
		SPDLOG_LOGGER_INFO(
		    appLogger,
		    "RHI smoke diagnostics capabilities: objectNames={} commandScopes={} timestampQueries={} debugMessages={} liveObjectReports={} crashDiagnostics={}",
		    capabilities.SupportsObjectNames,
		    capabilities.SupportsGpuEvents,
		    capabilities.SupportsTimestampQueries,
		    capabilities.SupportsDebugMessages,
		    capabilities.SupportsLiveObjectReports,
		    capabilities.SupportsCrashDiagnostics);

		state.DiagnosticsLogged = true;
	}

	void LogRendererSmokeEvidence(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (!config.Enabled || state.RendererEvidenceLogged)
		{
			return;
		}

		static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
		if (appLogger == nullptr)
		{
			return;
		}

		const RendererSmokeDiagnosticsSnapshot snapshot = app.GetRenderer().CaptureSmokeDiagnostics();
		SPDLOG_LOGGER_INFO(
		    appLogger,
		    "RHI editor smoke evidence: backend={} frameGraphUnresolvedBarrierWarnings={} upscalerProvider='{}' "
		    "upscalerStatus={} upscalerReason='{}' rayTracing={} inlineRayQuery={} tlasValid={} tlasInstances={}",
		    RhiBackendApiToString(snapshot.BackendApi),
		    snapshot.FrameGraphUnresolvedBarrierWarnings,
		    snapshot.UpscalerProvider,
		    snapshot.UpscalerStatus,
		    snapshot.UpscalerReason,
		    snapshot.RayTracingSupported,
		    snapshot.InlineRayQuerySupported,
		    snapshot.RayTracingTlasValid,
		    snapshot.RayTracingTlasInstanceCount);

		if (snapshot.FrameGraphUnresolvedBarrierWarnings > 0)
		{
			state.Failed = true;
			SPDLOG_LOGGER_ERROR(
			    appLogger,
			    "RHI editor smoke validation: frame graph reported {} unresolved barrier warning(s).",
			    snapshot.FrameGraphUnresolvedBarrierWarnings);
		}

		state.RendererEvidenceLogged = true;
	}

	void InitializeLevelSwitching(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (!config.Enabled || !config.LevelSwitching || state.LevelSwitchingInitialized)
		{
			return;
		}

		state.LevelSwitchingInitialized = true;
		static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
		LevelManager* levelManager = app.GetLevelManager();
		if (levelManager == nullptr)
		{
			state.Failed = true;
			SPDLOG_LOGGER_ERROR(appLogger, "RHI smoke validation: level switching requested but no LevelManager is available");
			return;
		}

		state.LevelSwitchOrder = levelManager->GetRegisteredLevelNames();
		const std::string activeLevelName = GetActiveLevelName(app);
		state.LevelSwitchOrder.erase(
		    std::remove(state.LevelSwitchOrder.begin(), state.LevelSwitchOrder.end(), activeLevelName),
		    state.LevelSwitchOrder.end());

		SPDLOG_LOGGER_INFO(
		    appLogger,
		    "RHI smoke validation: level switching initialized activeLevel='{}' switchTargets={}",
		    activeLevelName,
		    state.LevelSwitchOrder.size());

		if (state.LevelSwitchOrder.empty())
		{
			state.LevelSwitchingFinished = true;
		}
	}

	void AdvanceLevelSwitching(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (!config.Enabled || !config.LevelSwitching || state.LevelSwitchingFinished)
		{
			return;
		}

		InitializeLevelSwitching(config, app, state);

		static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
		const std::string activeLevelName = GetActiveLevelName(app);
		if (!state.PendingLevelName.empty() && activeLevelName == state.PendingLevelName)
		{
			++state.CompletedLevelSwitches;
			SPDLOG_LOGGER_INFO(
			    appLogger,
			    "RHI smoke validation: completed level switch to '{}' ({}/{})",
			    activeLevelName,
			    state.CompletedLevelSwitches,
			    state.LevelSwitchOrder.size());
			state.PendingLevelName.clear();
			state.LastLevelSwitchFrame = state.CompletedRenderFrames;
		}

		if (state.PendingLevelName.empty() && state.CompletedLevelSwitches >= state.LevelSwitchOrder.size())
		{
			state.LevelSwitchingFinished = true;
			SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: completed all level switch targets");
			return;
		}

		if (!state.PendingLevelName.empty())
		{
			return;
		}

		const std::uint32_t interval = std::max<std::uint32_t>(config.LevelSwitchIntervalFrames, 1u);
		if (state.CompletedRenderFrames - state.LastLevelSwitchFrame < interval)
		{
			return;
		}

		const std::string& nextLevelName = state.LevelSwitchOrder[state.CompletedLevelSwitches];
		LevelManager* levelManager = app.GetLevelManager();
		if (levelManager == nullptr)
		{
			state.Failed = true;
			SPDLOG_LOGGER_ERROR(appLogger, "RHI smoke validation: lost LevelManager before requesting level switch to '{}'", nextLevelName);
			return;
		}

		state.PendingLevelName = nextLevelName;
		SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: requesting level switch to '{}'", nextLevelName);
		levelManager->RequestLevelChange(nextLevelName);
	}

	void LogEditorViewportEvidence(
	    const EditorSmokeConfig& config,
	    const ViewportRenderProducts& viewportProducts,
	    const ViewportPresentationProduct& sceneColorPresentation,
	    EditorSmokeState& state) noexcept
	{
		if (!config.Enabled || state.EditorViewportEvidenceLogged)
		{
			return;
		}

		static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
		if (appLogger == nullptr)
		{
			return;
		}

		const RenderProduct& sceneColor = viewportProducts.GetSceneColor();
		SPDLOG_LOGGER_INFO(
		    appLogger,
		    "RHI editor smoke evidence: viewport sceneColorHandle={} textureId={} extent={}x{} outputsMask={} presentationStatus={} reason='{}'",
		    sceneColor.Handle.Value,
		    sceneColorPresentation.TextureId,
		    sceneColor.Extent.Width,
		    sceneColor.Extent.Height,
		    static_cast<std::uint32_t>(viewportProducts.GetAvailableOutputs()),
		    static_cast<std::uint32_t>(sceneColorPresentation.Status),
		    sceneColorPresentation.FailureReason);

		state.EditorViewportEvidenceLogged = true;
	}

	void CaptureEditorSceneColorIfRequested(
	    const EditorSmokeConfig& config,
	    RuntimeApplication& app,
	    EditorSmokeState& state) noexcept
	{
		if (!config.Enabled || state.SceneColorCaptured || config.SceneColorCapturePath.empty())
		{
			return;
		}

		const std::uint32_t currentFrame = state.CompletedRenderFrames + 1u;
		if (currentFrame < std::max<std::uint32_t>(config.SceneColorCaptureFrame, 1u))
		{
			return;
		}

		static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
		Renderer& renderer = app.GetRenderer();
		const RenderViewMode viewMode = config.HasViewModeOverride ? config.ViewModeOverride : CVarRenderViewMode.Get();
		const RhiCaptureResult captureResult = renderer.CaptureViewportProductToBmp(
		    ViewportCaptureRequest{
		        .Output = RenderOutputFlags::SceneColor,
		        .OutputPath = std::filesystem::path(config.SceneColorCapturePath),
		        .FrameIndex = currentFrame,
		        .ViewMode = static_cast<std::uint32_t>(viewMode),
		        .ViewModeName = RenderViewModeName(viewMode),
		        .DebugName = "Editor smoke scene color"});

		if (captureResult)
		{
			SPDLOG_LOGGER_INFO(
			    appLogger,
			    "RHI editor smoke: captured scene color backend={} viewMode={}({}) path='{}' frame={}",
			    RhiBackendApiToString(captureResult.BackendApi),
			    captureResult.ViewModeName,
			    captureResult.ViewMode,
			    captureResult.ArtifactPath.string(),
			    captureResult.FrameIndex);
			state.SceneColorCaptured = true;
		}
		else
		{
			SPDLOG_LOGGER_ERROR(
			    appLogger,
			    "RHI editor smoke: failed to capture scene color backend={} viewMode={}({}) path='{}' frame={} reason='{}'",
			    RhiBackendApiToString(captureResult.BackendApi),
			    captureResult.ViewModeName,
			    captureResult.ViewMode,
			    config.SceneColorCapturePath,
			    captureResult.FrameIndex,
			    captureResult.FailureReason);
			state.Failed = true;
			state.SceneColorCaptured = true;
		}
	}

	void Advance(const EditorSmokeConfig& config, RuntimeApplication& app, EditorSmokeState& state) noexcept
	{
		if (!config.Enabled)
		{
			return;
		}

		Window& window = app.GetWindow();
		++state.CompletedRenderFrames;
		static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
		AdvanceLevelSwitching(config, app, state);
		RhiSmokeCameraMotion::Advance(config.CameraMotion, app, state.CompletedRenderFrames, state.CameraMotion);

		if (config.ShaderReloadFrame > 0 && state.CompletedRenderFrames == config.ShaderReloadFrame)
		{
			Renderer& renderer = app.GetRenderer();
			renderer.GetRenderHardwareInterface().WaitForIdle();
			const CookedShaderReloadResult reloadResult = renderer.ReloadCookedShaders();
			if (appLogger != nullptr)
			{
				if (reloadResult)
				{
					SPDLOG_LOGGER_INFO(
					    appLogger,
					    "RHI smoke validation: reloaded cooked shaders on frame {} (generation={})",
					    state.CompletedRenderFrames,
					    renderer.GetShaderPackageGeneration());
				}
				else
				{
					SPDLOG_LOGGER_ERROR(
					    appLogger,
					    "RHI smoke validation: cooked shader reload was rejected on frame {}. {}",
					    state.CompletedRenderFrames,
					    reloadResult.ErrorMessage);
				}
			}
		}

		if (config.RestoreFrame > 0 && state.CompletedRenderFrames == config.RestoreFrame)
		{
			if (appLogger != nullptr)
			{
				SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: restoring window on frame {}", state.CompletedRenderFrames);
			}
			window.Restore();
		}

		if (config.MaximizeFrame > 0 && state.CompletedRenderFrames == config.MaximizeFrame)
		{
			if (appLogger != nullptr)
			{
				SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: maximizing window on frame {}", state.CompletedRenderFrames);
			}
			window.Maximize();
		}

		if (config.FrameLimit > 0 && state.CompletedRenderFrames >= config.FrameLimit)
		{
			if (!RhiSmokeCameraMotion::Validate(config.CameraMotion, state.CameraMotion, app.GetRenderer().CaptureSmokeDiagnostics(), "editor"))
			{
				state.Failed = true;
				SPDLOG_LOGGER_ERROR(appLogger, "RHI smoke validation: camera motion evidence failed validation");
			}

			if (config.LevelSwitching && !state.LevelSwitchingFinished)
			{
				state.Failed = true;
				SPDLOG_LOGGER_ERROR(
				    appLogger,
				    "RHI smoke validation: frame limit {} reached before level switching completed ({}/{})",
				    config.FrameLimit,
				    state.CompletedLevelSwitches,
				    state.LevelSwitchOrder.size());
			}

			if (appLogger != nullptr)
			{
				SPDLOG_LOGGER_INFO(appLogger, "RHI smoke validation: reached frame limit {}, requesting shutdown", config.FrameLimit);
			}
			window.RequestClose();
		}
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
		if (config.HasViewModeOverride)
		{
			CVarRenderViewMode.Set(config.ViewModeOverride);
			if (!state.ViewModeOverrideLogged)
			{
				static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");
				SPDLOG_LOGGER_INFO(
				    appLogger,
				    "RHI editor smoke: forced render view mode {}",
				    static_cast<std::uint32_t>(config.ViewModeOverride));
				state.ViewModeOverrideLogged = true;
			}
		}
		renderer.PrepareHostFrame();
		renderer.RecordHostFrame();
		LogRendererSmokeEvidence(config, app, state);

		const ViewportRenderProducts& viewportProducts = app.GetViewportRenderProducts();
		ui.SetViewportRenderProducts(viewportProducts);
		const ViewportPresentationProduct sceneColorPresentation = renderer.BeginViewportPresentation(RenderOutputFlags::SceneColor);
		ui.SetViewportSceneColorTextureId(sceneColorPresentation.TextureId);
		LogEditorViewportEvidence(config, viewportProducts, sceneColorPresentation, state);
		ui.Update();

		RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();

		constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
		RhiPresentationService& presentationService = renderHardware.GetPresentationService();
		presentationService.BeginPresentRenderPass(editorClearColor);
		ui.Render();
		presentationService.EndPresentRenderPass();

		renderer.EndViewportPresentation(RenderOutputFlags::SceneColor);

		renderer.SubmitHostFrame();
		CaptureEditorSceneColorIfRequested(config, app, state);
		app.EndFrame();
		Advance(config, app, state);
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
	InitializeLevelSwitching(config, app, state);
	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");

	{
		SPARKLE_LOG_SCOPE(appLogger, spdlog::level::info, "RHI editor smoke UI scope");
		Renderer& renderer = app.GetRenderer();
		app.GetInputSystem().ClearInputCaptureQuery();
		app.GetInputSystem().BeginInputRoutingFrame(false, false);
		UI ui(EditorHostServices{
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
	return state.Failed ? 1 : 0;
}
