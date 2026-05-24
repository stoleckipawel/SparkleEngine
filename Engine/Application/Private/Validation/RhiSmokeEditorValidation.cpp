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
#include "RuntimeApplication.h"

#include <algorithm>
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
	};

	struct EditorSmokeState final
	{
		std::uint32_t CompletedRenderFrames = 0;
		bool DiagnosticsLogged = false;
		bool EditorViewportEvidenceLogged = false;
		bool LevelSwitchingInitialized = false;
		bool LevelSwitchingFinished = false;
		bool Failed = false;
		std::uint32_t LastLevelSwitchFrame = 0;
		std::uint32_t CompletedLevelSwitches = 0;
		std::vector<std::string> LevelSwitchOrder;
		std::string PendingLevelName;
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
		return config;
	}

	void ApplyLoggingConfig(const EditorSmokeConfig& config) noexcept
	{
		if (config.Enabled && config.TraceLogging)
		{
			Logging::SetLevel(spdlog::level::trace);
		}
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
		const RhiDiagnosticsCapabilities capabilities = renderHardware.GetDiagnostics().GetCapabilities();
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
	    RuntimeApplication& app,
	    const ViewportRenderProducts& viewportProducts,
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

		Renderer& renderer = app.GetRenderer();
		const RenderProduct& sceneColor = viewportProducts.GetSceneColor();
		const std::uint64_t sceneColorTextureId = renderer.ResolveRenderProductTextureId(sceneColor.Handle);
		SPDLOG_LOGGER_INFO(
		    appLogger,
		    "RHI editor smoke evidence: viewport sceneColorHandle={} textureId={} extent={}x{} outputsMask={}",
		    sceneColor.Handle.Value,
		    sceneColorTextureId,
		    sceneColor.Extent.Width,
		    sceneColor.Extent.Height,
		    static_cast<std::uint32_t>(viewportProducts.GetAvailableOutputs()));

		state.EditorViewportEvidenceLogged = true;
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
		renderer.PrepareHostFrame();
		renderer.RecordHostFrame();

		const ViewportRenderProducts& viewportProducts = app.GetViewportRenderProducts();
		ui.SetViewportRenderProducts(viewportProducts);
		ui.SetViewportSceneColorTextureId(renderer.ResolveRenderProductTextureId(viewportProducts.GetSceneColor().Handle));
		LogEditorViewportEvidence(config, app, viewportProducts, state);
		ui.Update();

		RenderHardwareInterface& renderHardware = renderer.GetRenderHardwareInterface();
		renderer.TransitionRenderProduct(
		    viewportProducts.GetSceneColor().Handle,
		    ResourceState::RenderTarget,
		    ResourceState::ShaderResource);

		constexpr float editorClearColor[4] = {0.06f, 0.06f, 0.07f, 1.0f};
		renderHardware.BeginPresentRenderPass(editorClearColor);
		ui.Render();
		renderHardware.EndPresentRenderPass();

		renderer.TransitionRenderProduct(
		    viewportProducts.GetSceneColor().Handle,
		    ResourceState::ShaderResource,
		    ResourceState::Common);

		renderer.SubmitHostFrame();
		app.EndFrame();
		Advance(config, app, state);
		return true;
	}
}

int RhiSmokeValidation::RunEditor() noexcept
{
	const EditorSmokeConfig config = LoadConfig();
	RuntimeApplication app;
	EditorSmokeState state{};
	ApplyLoggingConfig(config);
	app.Initialize();
	LogDiagnosticsCapabilities(config, app, state);
	InitializeLevelSwitching(config, app, state);
	static const auto appLogger = Logging::GetOrCreateLogger("Application.SmokeValidation");

	{
		SPARKLE_LOG_SCOPE(appLogger, spdlog::level::info, "RHI editor smoke UI scope");
		Renderer& renderer = app.GetRenderer();
		app.GetInputSystem().SetAutomaticImGuiCaptureEnabled(false);
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
		app.GetInputSystem().SetAutomaticImGuiCaptureEnabled(true);
	}

	app.Shutdown();
	SPDLOG_LOGGER_INFO(appLogger, "RHI editor smoke: RuntimeApplication shutdown complete");
	return state.Failed ? 1 : 0;
}