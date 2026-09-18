#include "PCH.h"

#include "Editor/ReferencePathTracer/ReferencePathTracerApplication.h"

#include "Application.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Hash/HashUtils.h"
#include "Core/Public/Json/JsonWriter.h"
#include "Core/Public/Paths/PathUtils.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerArtifactCoordinator.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerOffscreenRequest.h"
#include "EditorOperations/EditorOperationRuntime.h"
#include "Level/LevelSession.h"
#include "Renderer.h"
#include "RuntimeApplication.h"

#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

static std::string Digest(const ViewportRenderProducts& products)
{
	const RenderProduct* raw = products.FindProduct(RenderOutputFlags::RawSceneColor);
	if (raw == nullptr)
	{
		return {};
	}
	return Hash::Sha256ToHex(raw->Source.IdentitySha256);
}

static void EmitProgress(const ViewportRenderProducts& products, std::uint64_t sequence)
{
	const ViewportRenderProgress& progress = products.GetProgress();
	constexpr std::array<std::string_view, 6> states = {"Inactive", "Resetting", "Accumulating", "Paused", "Unavailable", "Complete"};
	std::cout << '{' << Json::QuoteString("committedSpp") << ':' << progress.CompletedWork << ',' << Json::QuoteString("digest") << ':'
	          << Json::QuoteString(Digest(products)) << ',' << Json::QuoteString("schema") << ':'
	          << Json::QuoteString("sparkle.reference-path-tracer.event/1") << ',' << Json::QuoteString("sequence") << ':' << sequence
	          << ',' << Json::QuoteString("state") << ':' << Json::QuoteString(states[static_cast<std::size_t>(progress.State)]) << ','
	          << Json::QuoteString("targetSpp") << ':' << progress.TargetWork << ',' << Json::QuoteString("type") << ':'
	          << Json::QuoteString("progress") << "}\n";
	std::cout.flush();
}

static void EmitTerminal(
    std::string_view status,
    int exitCode,
    std::string_view digest,
    const std::filesystem::path& publication,
    std::string_view manifestSha256,
    bool checkpoint,
    std::string_view reason)
{
	const std::string manifest =
	    publication.empty() ? std::string("null") : Json::QuoteString(Paths::ToUtf8String(publication / "manifest.json"));
	const std::string checkpointPath =
	    publication.empty() || !checkpoint ? std::string("null") : Json::QuoteString(Paths::ToUtf8String(publication / "checkpoint.bin"));
	std::cout << '{' << Json::QuoteString("checkpointPath") << ':' << checkpointPath << ',' << Json::QuoteString("digest") << ':'
	          << (digest.empty() ? "null" : Json::QuoteString(digest)) << ',' << Json::QuoteString("exitCode") << ':' << exitCode << ','
	          << Json::QuoteString("manifestPath") << ':' << manifest << ',' << Json::QuoteString("manifestSha256") << ':'
	          << (manifestSha256.empty() ? "null" : Json::QuoteString(manifestSha256)) << ',' << Json::QuoteString("reasonCode") << ':'
	          << Json::QuoteString(reason) << ',' << Json::QuoteString("schema") << ':'
	          << Json::QuoteString("sparkle.reference-path-tracer.event/1") << ',' << Json::QuoteString("status") << ':'
	          << Json::QuoteString(status) << ',' << Json::QuoteString("type") << ':' << Json::QuoteString("terminal") << "}\n";
	std::cout.flush();
}

static int RunOffscreen(const ReferencePathTracerOffscreenRequest& description)
{
	if (ResolveDefaultRhiBackendApi() != description.Backend)
	{
		EmitTerminal("InvalidRequest", 2, {}, {}, {}, false, "backend-mismatch");
		return 2;
	}
	RuntimeApplication runtime(
	    RuntimeApplicationOptions{
	        .EnableRuntimeConsole = false,
	        .AllowThreadedRenderer = false,
	        .EnableUiRenderPackets = false,
	        .WindowVisible = false,
	        .WindowWidth = description.Width,
	        .WindowHeight = description.Height});
	runtime.Initialize();
	if (runtime.GetLevelSession() != nullptr)
	{
		runtime.GetLevelSession()->RequestLevelChange(description.LevelId);
	}
	Renderer& renderer = runtime.GetRenderer();
	int exitCode = 5;
	std::string terminalStatus = "TimedOut";
	std::string terminalReason = "timeout";
	std::string terminalDigest;
	std::filesystem::path terminalPublication;
	std::string terminalManifestSha256;
	bool terminalCheckpoint = false;
	{
		EditorOperationRuntime operationRuntime(runtime.GetTaskExecutor(), runtime.GetApplicationTaskScope());
		ReferencePathTracerArtifactCoordinator artifacts(operationRuntime);
		ViewportRenderRequest request{
		    .ViewportId = 1u,
		    .Generation = 1u,
		    .ViewKind = RenderViewKind::Game,
		    .ViewMode = RenderViewMode::ReferencePathTracer,
		    .Extent = RenderViewportExtent{description.Width, description.Height},
		    .RequestedOutputs = RenderOutputFlags::SceneColor};
		runtime.SubmitViewportRenderRequest(request);

		bool publicationRequested = false;
		bool timeoutCheckpointRequested = false;
		std::uint64_t lastReportedPrefix = (std::numeric_limits<std::uint64_t>::max)();
		std::uint64_t eventSequence = 0u;
		const auto started = std::chrono::steady_clock::now();
		auto checkpointDeadline = std::chrono::steady_clock::time_point::max();
		for (;;)
		{
			const RuntimeApplicationFrameResult frameResult = runtime.BeginFrame();
			if (frameResult == RuntimeApplicationFrameResult::Exit)
			{
				exitCode = 4;
				terminalStatus = "Cancelled";
				terminalReason = "window-closed";
				break;
			}
			if (frameResult == RuntimeApplicationFrameResult::SkipRender)
			{
				continue;
			}
			runtime.UpdateRuntime();
			renderer.OnRender();

			const ViewportRenderProducts products = runtime.GetViewportRenderProducts();
			artifacts.Update(renderer, products);
			const ViewportRenderProgress& progress = products.GetProgress();
			terminalDigest = Digest(products);
			if (progress.CompletedWork != lastReportedPrefix)
			{
				lastReportedPrefix = progress.CompletedWork;
				EmitProgress(products, eventSequence++);
			}
			if (progress.State == ViewportRenderProgressState::Unavailable)
			{
				exitCode = progress.Reason == ViewportRenderProgressReason::SessionCapacity ? 6 : 3;
				terminalStatus =
				    progress.Reason == ViewportRenderProgressReason::SessionCapacity ? "CapacityExceeded" : "UnsupportedCapability";
				terminalReason = "renderer-unavailable";
				break;
			}
			const auto now = std::chrono::steady_clock::now();
			if (!publicationRequested && now - started >= std::chrono::seconds(description.TimeoutSeconds))
			{
				if (!description.CheckpointOnTimeout || progress.CompletedWork == 0u)
				{
					exitCode = 5;
					terminalStatus = "TimedOut";
					terminalReason = "timeout";
					break;
				}
				publicationRequested = true;
				timeoutCheckpointRequested = true;
				checkpointDeadline = now + std::chrono::seconds(30);
				request.RenderAction = ViewportRenderAction::Pause;
				++request.RenderActionSequence;
				++request.Generation;
				runtime.SubmitViewportRenderRequest(request);
				artifacts.Request(
				    ReferencePathTracerOutputAction::SaveCheckpoint,
				    renderer,
				    products,
				    description.OutputDirectory,
				    description.MaximumOutputBytes);
			}
			if (timeoutCheckpointRequested && now >= checkpointDeadline && !artifacts.IsSettled())
			{
				exitCode = 8;
				terminalStatus = "CheckpointRejected";
				terminalReason = "checkpoint-timeout";
				break;
			}
			if (!publicationRequested && progress.State == ViewportRenderProgressState::Complete)
			{
				publicationRequested = true;
				artifacts.Request(
				    ReferencePathTracerOutputAction::SaveComplete,
				    renderer,
				    products,
				    description.OutputDirectory,
				    description.MaximumOutputBytes);
			}
			if (publicationRequested && artifacts.IsSettled())
			{
				exitCode = artifacts.GetLastResult().Succeeded ? (timeoutCheckpointRequested ? 5 : 0) : 8;
				terminalStatus = artifacts.GetLastResult().Succeeded
				    ? (timeoutCheckpointRequested ? "TimedOut" : "Completed")
				    : (timeoutCheckpointRequested ? "CheckpointRejected" : "PublicationFailed");
				terminalReason = artifacts.GetLastResult().Succeeded
				    ? (timeoutCheckpointRequested ? "timeout-checkpoint-published" : "completed")
				    : (timeoutCheckpointRequested ? "checkpoint-publication-failed" : "publication-failed");
				terminalPublication = artifacts.GetLastResult().PublicationDirectory;
				terminalManifestSha256 = artifacts.GetLastResult().ManifestSha256;
				terminalCheckpoint = timeoutCheckpointRequested && artifacts.GetLastResult().Succeeded;
				break;
			}
		}
	}
	runtime.Shutdown();
	EmitTerminal(terminalStatus, exitCode, terminalDigest, terminalPublication, terminalManifestSha256, terminalCheckpoint, terminalReason);
	return exitCode;
}

static int RunReferencePathTracerOffscreenManifest(const std::filesystem::path& manifestPath)
{
#if SPARKLE_BUILD_SHIPPING
	(void) manifestPath;
	return 7;
#else
	try
	{
		Application::ConfigureProcessFromCommandLine();
		ReferencePathTracerOffscreenRequest request;
		std::string errorMessage;
		if (!ReadReferencePathTracerOffscreenRequest(manifestPath, request, errorMessage))
		{
			EmitTerminal("InvalidRequest", 2, {}, {}, {}, false, errorMessage);
			return 2;
		}
		Filesystem::ConfigureProjectRoot(request.ProjectRoot);
		return RunOffscreen(request);
	}
	catch (...)
	{
		EmitTerminal("InternalFailure", 9, {}, {}, {}, false, "unhandled-host-error");
		return 9;
	}
#endif
}

std::optional<int> TryRunReferencePathTracerApplication(int argumentCount, wchar_t* arguments[])
{
	for (int index = 1; index < argumentCount; ++index)
	{
		if (std::wstring_view(arguments[index]) == L"--reference-path-tracer-request")
		{
			return index + 1 < argumentCount ? RunReferencePathTracerOffscreenManifest(std::filesystem::path(arguments[index + 1])) : 2;
		}
	}
	return std::nullopt;
}
