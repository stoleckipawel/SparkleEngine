#pragma once

#include "Editor/Capture/ViewportCaptureSlot.h"
#include "EditorOperations/EditorOperationSlot.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerArtifact.h"

#include <filesystem>
#include <optional>

class EditorOperationRuntime;
class Renderer;

class ReferencePathTracerArtifactCoordinator final
{
public:
	explicit ReferencePathTracerArtifactCoordinator(EditorOperationRuntime& operations) noexcept;

	void Request(
	    ReferencePathTracerArtifactKind kind,
	    Renderer& renderer,
	    const ViewportRenderProducts& products,
	    const std::filesystem::path& outputRoot = {},
	    std::uint64_t maximumOutputBytes = 64ull * 1024ull * 1024ull * 1024ull);
	void Update(Renderer& renderer, const ViewportRenderProducts& products);

	bool IsSettled() const noexcept;
	const ReferencePathTracerArtifactWriteResult& GetLastResult() const noexcept { return m_lastResult; }

private:
	void Begin(
	    ReferencePathTracerArtifactKind kind,
	    Renderer& renderer,
	    const ViewportRenderProducts& products,
	    const std::filesystem::path& outputRoot);
	void CollectReadback(Renderer& renderer, ViewportCaptureSlot& capture, std::optional<ViewportCaptureReadback>& destination);
	void PublishIfReady();
	void Fail(std::string message);
	static std::filesystem::path DefaultOutputRoot();

	EditorOperationSlot<ReferencePathTracerArtifactWriteResult> m_writeOperation;
	ReferencePathTracerArtifactKind m_kind = ReferencePathTracerArtifactKind::PartialPrefix;
	std::filesystem::path m_outputRoot;
	std::filesystem::path m_publicationDirectory;
	std::uint64_t m_maximumOutputBytes = 64ull * 1024ull * 1024ull * 1024ull;
	ViewportCaptureSlot m_meanCapture;
	ViewportCaptureSlot m_moment2Capture;
	std::optional<ViewportCaptureReadback> m_mean;
	std::optional<ViewportCaptureReadback> m_moment2;
	std::optional<ReferencePathTracerArtifactKind> m_pendingKind;
	ReferencePathTracerArtifactWriteResult m_lastResult;
	RenderProductSamplePrefix m_pendingPrefix = {};
	RenderProductSamplePrefix m_capturePrefix = {};
};
