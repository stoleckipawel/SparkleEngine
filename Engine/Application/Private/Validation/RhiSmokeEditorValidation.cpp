#include "PCH.h"

#include "Validation/RhiSmokeValidation.h"

#include "Core/Public/Environment/EnvironmentVariables.h"
#include "Diagnostics/ScopedLogEvent.h"
#include "Editor/Public/UI.h"
#include "Input/InputSystem.h"
#include "Level/Level.h"
#include "Level/LevelManager.h"
#include "Platform/Public/Window/Window.h"
#include "RHI/Public/Core/RhiBackendApi.h"
#include "Renderer.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "RuntimeApplication.h"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <d3d12.h>
#include <wrl/client.h>
#endif

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
		bool SceneColorCaptured = false;
		bool ViewModeOverrideLogged = false;
	};

#if defined(_WIN32)
#pragma pack(push, 1)
	struct BmpFileHeader final
	{
		std::uint16_t Type = 0x4D42;
		std::uint32_t Size = 0;
		std::uint16_t Reserved1 = 0;
		std::uint16_t Reserved2 = 0;
		std::uint32_t OffBits = 54;
	};

	struct BmpInfoHeader final
	{
		std::uint32_t Size = sizeof(BmpInfoHeader);
		std::int32_t Width = 0;
		std::int32_t Height = 0;
		std::uint16_t Planes = 1;
		std::uint16_t BitCount = 32;
		std::uint32_t Compression = 0;
		std::uint32_t SizeImage = 0;
		std::int32_t XPelsPerMeter = 2835;
		std::int32_t YPelsPerMeter = 2835;
		std::uint32_t ClrUsed = 0;
		std::uint32_t ClrImportant = 0;
	};
#pragma pack(pop)

	bool WriteSceneColorBmp(
	    const std::filesystem::path& outputPath,
	    const std::byte* sourcePixels,
	    std::uint32_t width,
	    std::uint32_t height,
	    std::uint32_t sourceRowPitch,
	    DXGI_FORMAT sourceFormat) noexcept
	{
		if (sourcePixels == nullptr || width == 0 || height == 0)
		{
			return false;
		}

		const bool sourceIsRgba =
		    sourceFormat == DXGI_FORMAT_R8G8B8A8_UNORM || sourceFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		const bool sourceIsBgra =
		    sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM || sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		if (!sourceIsRgba && !sourceIsBgra)
		{
			return false;
		}

		std::error_code error;
		if (const std::filesystem::path parentPath = outputPath.parent_path(); !parentPath.empty())
		{
			std::filesystem::create_directories(parentPath, error);
			if (error)
			{
				return false;
			}
		}

		const std::uint32_t outputRowPitch = width * 4u;
		std::vector<std::byte> outputPixels(static_cast<std::size_t>(outputRowPitch) * height);
		for (std::uint32_t y = 0; y < height; ++y)
		{
			const std::byte* sourceRow = sourcePixels + static_cast<std::size_t>(sourceRowPitch) * y;
			std::byte* outputRow = outputPixels.data() + static_cast<std::size_t>(outputRowPitch) * y;
			for (std::uint32_t x = 0; x < width; ++x)
			{
				const std::byte* sourcePixel = sourceRow + static_cast<std::size_t>(x) * 4u;
				std::byte* outputPixel = outputRow + static_cast<std::size_t>(x) * 4u;
				if (sourceIsRgba)
				{
					outputPixel[0] = sourcePixel[2];
					outputPixel[1] = sourcePixel[1];
					outputPixel[2] = sourcePixel[0];
					outputPixel[3] = sourcePixel[3];
				}
				else
				{
					outputPixel[0] = sourcePixel[0];
					outputPixel[1] = sourcePixel[1];
					outputPixel[2] = sourcePixel[2];
					outputPixel[3] = sourcePixel[3];
				}
			}
		}

		BmpFileHeader fileHeader{};
		BmpInfoHeader infoHeader{};
		infoHeader.Width = static_cast<std::int32_t>(width);
		infoHeader.Height = -static_cast<std::int32_t>(height);
		infoHeader.SizeImage = static_cast<std::uint32_t>(outputPixels.size());
		fileHeader.Size = fileHeader.OffBits + infoHeader.SizeImage;

		std::ofstream output(outputPath, std::ios::binary);
		if (!output)
		{
			return false;
		}

		output.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
		output.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
		output.write(reinterpret_cast<const char*>(outputPixels.data()), static_cast<std::streamsize>(outputPixels.size()));
		return output.good();
	}

	bool CaptureD3D12SceneColor(RenderHardwareInterface& renderHardware, NativeResourceHandle resourceHandle, const std::filesystem::path& outputPath)
	    noexcept
	{
		if (renderHardware.GetBackendApi() != ERhiBackendApi::D3D12 || !resourceHandle)
		{
			return false;
		}

		const RhiNativeDeviceQueueInterop interop = renderHardware.GetInteropService().GetDeviceQueueInterop(
		    RhiNativeInteropRequest{
		        .Consumer = ERhiNativeInteropConsumer::Validation,
		        .Reason = "D3D12 editor smoke capture requires native device and graphics queue access."});
		ID3D12Device* device = static_cast<ID3D12Device*>(interop.Device.Value);
		ID3D12CommandQueue* queue = static_cast<ID3D12CommandQueue*>(interop.GraphicsQueue.Value);
		ID3D12Resource* sourceResource = static_cast<ID3D12Resource*>(resourceHandle.Value);
		if (device == nullptr || queue == nullptr || sourceResource == nullptr)
		{
			return false;
		}

		const D3D12_RESOURCE_DESC sourceDesc = sourceResource->GetDesc();
		if (sourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D || sourceDesc.Width == 0 || sourceDesc.Height == 0)
		{
			return false;
		}

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
		UINT rowCount = 0;
		UINT64 rowSizeInBytes = 0;
		UINT64 totalBytes = 0;
		device->GetCopyableFootprints(&sourceDesc, 0, 1, 0, &footprint, &rowCount, &rowSizeInBytes, &totalBytes);
		if (totalBytes == 0 || rowCount == 0)
		{
			return false;
		}

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 1;
		heapProperties.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC readbackDesc{};
		readbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		readbackDesc.Alignment = 0;
		readbackDesc.Width = totalBytes;
		readbackDesc.Height = 1;
		readbackDesc.DepthOrArraySize = 1;
		readbackDesc.MipLevels = 1;
		readbackDesc.Format = DXGI_FORMAT_UNKNOWN;
		readbackDesc.SampleDesc.Count = 1;
		readbackDesc.SampleDesc.Quality = 0;
		readbackDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		readbackDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer;
		if (FAILED(device->CreateCommittedResource(
		        &heapProperties,
		        D3D12_HEAP_FLAG_NONE,
		        &readbackDesc,
		        D3D12_RESOURCE_STATE_COPY_DEST,
		        nullptr,
		        IID_PPV_ARGS(&readbackBuffer))))
		{
			return false;
		}

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
		if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))))
		{
			return false;
		}

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
		if (FAILED(device->CreateCommandList(
		        0,
		        D3D12_COMMAND_LIST_TYPE_DIRECT,
		        commandAllocator.Get(),
		        nullptr,
		        IID_PPV_ARGS(&commandList))))
		{
			return false;
		}

		D3D12_RESOURCE_BARRIER toCopySource{};
		toCopySource.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		toCopySource.Transition.pResource = sourceResource;
		toCopySource.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		toCopySource.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		toCopySource.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
		commandList->ResourceBarrier(1, &toCopySource);

		D3D12_TEXTURE_COPY_LOCATION sourceLocation{};
		sourceLocation.pResource = sourceResource;
		sourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		sourceLocation.SubresourceIndex = 0;

		D3D12_TEXTURE_COPY_LOCATION destinationLocation{};
		destinationLocation.pResource = readbackBuffer.Get();
		destinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		destinationLocation.PlacedFootprint = footprint;
		commandList->CopyTextureRegion(&destinationLocation, 0, 0, 0, &sourceLocation, nullptr);

		std::swap(toCopySource.Transition.StateBefore, toCopySource.Transition.StateAfter);
		commandList->ResourceBarrier(1, &toCopySource);

		if (FAILED(commandList->Close()))
		{
			return false;
		}

		ID3D12CommandList* commandLists[] = {commandList.Get()};
		queue->ExecuteCommandLists(1, commandLists);

		Microsoft::WRL::ComPtr<ID3D12Fence> fence;
		if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
		{
			return false;
		}

		const UINT64 fenceValue = 1;
		if (FAILED(queue->Signal(fence.Get(), fenceValue)))
		{
			return false;
		}

		if (fence->GetCompletedValue() < fenceValue)
		{
			const HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (fenceEvent == nullptr)
			{
				return false;
			}

			if (FAILED(fence->SetEventOnCompletion(fenceValue, fenceEvent)))
			{
				CloseHandle(fenceEvent);
				return false;
			}

			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);
		}

		void* mappedData = nullptr;
		D3D12_RANGE readRange{0, static_cast<SIZE_T>(totalBytes)};
		if (FAILED(readbackBuffer->Map(0, &readRange, &mappedData)) || mappedData == nullptr)
		{
			return false;
		}

		const std::byte* sourcePixels = static_cast<const std::byte*>(mappedData) + footprint.Offset;
		const bool wroteCapture = WriteSceneColorBmp(
		    outputPath,
		    sourcePixels,
		    footprint.Footprint.Width,
		    footprint.Footprint.Height,
		    footprint.Footprint.RowPitch,
		    sourceDesc.Format);
		D3D12_RANGE writeRange{0, 0};
		readbackBuffer->Unmap(0, &writeRange);
		return wroteCapture;
	}
#endif

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

	void CaptureEditorSceneColorIfRequested(
	    const EditorSmokeConfig& config,
	    RuntimeApplication& app,
	    const ViewportRenderProducts& viewportProducts,
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
		const NativeResourceHandle sceneColorResource =
		    renderer.ResolveRenderProductResource(viewportProducts.GetSceneColor().Handle);

		const RenderProduct& sceneColorProduct = viewportProducts.GetSceneColor();
		const RhiCaptureResult captureResult = renderer.GetRenderHardwareInterface().GetCaptureService().CaptureTextureToBmp(
		    RhiTextureCaptureRequest{
		        .Resource = sceneColorResource,
		        .Width = sceneColorProduct.Extent.Width,
		        .Height = sceneColorProduct.Extent.Height,
		        .OutputPath = std::filesystem::path(config.SceneColorCapturePath),
		        .DebugName = "Editor smoke scene color"});

		if (captureResult)
		{
			SPDLOG_LOGGER_INFO(
			    appLogger,
			    "RHI editor smoke: captured scene color for visual validation path='{}' frame={}",
			    config.SceneColorCapturePath,
			    currentFrame);
			state.SceneColorCaptured = true;
		}
		else
		{
			SPDLOG_LOGGER_ERROR(
			    appLogger,
			    "RHI editor smoke: failed to capture scene color path='{}' frame={}",
			    config.SceneColorCapturePath,
			    currentFrame);
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
		RhiPresentationService& presentationService = renderHardware.GetPresentationService();
		presentationService.BeginPresentRenderPass(editorClearColor);
		ui.Render();
		presentationService.EndPresentRenderPass();

		renderer.TransitionRenderProduct(
		    viewportProducts.GetSceneColor().Handle,
		    ResourceState::ShaderResource,
		    ResourceState::Common);

		renderer.SubmitHostFrame();
		CaptureEditorSceneColorIfRequested(config, app, viewportProducts, state);
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
