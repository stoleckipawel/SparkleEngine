#include "PCH.h"
#include "Renderer.h"

#include "Concurrency/Coordinator/RenderCoordinator.h"
#include "GameFramework/Public/Rendering/RenderFrameSubmission.h"
#include "Integrations/RendererExternalRuntime.h"

class RendererFacadeState final
{
public:
	RendererFacadeState(Timer& timer, Window& window, RendererGraphicsLaunch graphicsLaunch, RendererExecutionConfig config) :
	    ExternalRuntime(graphicsLaunch),
	    Coordinator(timer, window, config, ExternalRuntime.GetDeviceLaunch())
	{
	}

	RendererExternalRuntime ExternalRuntime;
	RenderCoordinator Coordinator;
};

Renderer::Renderer(Timer& timer, Window& window, RendererGraphicsLaunch graphicsLaunch, RendererExecutionConfig config) noexcept :
    m_state(std::make_unique<RendererFacadeState>(timer, window, graphicsLaunch, config))
{
}

Renderer::~Renderer() noexcept = default;

void Renderer::SubmitViewportRenderRequest(ViewportRenderRequest request) noexcept
{
	m_state->Coordinator.SubmitViewportRequest(request);
}

ViewportPresentationSnapshot Renderer::GetViewportPresentation() const
{
	return m_state->Coordinator.GetViewportPresentation();
}

EngineRenderingSettingsState Renderer::CaptureRenderingSettings() const
{
	return m_state->Coordinator.CaptureRenderingSettings();
}

CVarControlResult Renderer::ExecuteConsoleVariables(CVarControlRequest request)
{
	return m_state->Coordinator.ExecuteConsoleVariables(std::move(request));
}

void Renderer::ReloadShaders()
{
	m_state->Coordinator.ReloadShaders();
}

std::uint64_t Renderer::GetShaderGeneration() const noexcept
{
	return m_state->Coordinator.GetShaderGeneration();
}

MeshDiagnosticsSnapshot Renderer::CaptureMeshDiagnostics() const
{
	return m_state->Coordinator.CaptureMeshDiagnostics();
}

TextureDiagnosticsSnapshot Renderer::CaptureTextureDiagnostics() const
{
	return m_state->Coordinator.CaptureTextureDiagnostics();
}

RendererMemoryDiagnosticsSnapshot Renderer::CaptureMemoryDiagnostics() const
{
	return m_state->Coordinator.CaptureMemoryDiagnostics();
}

MeshPreviewGeometry Renderer::CaptureMeshPreview(std::uintptr_t meshRuntimeId) const
{
	return m_state->Coordinator.CaptureMeshPreview(meshRuntimeId);
}

void Renderer::SubmitRenderFrame(RenderFrameSubmission submission) noexcept
{
	m_state->Coordinator.StageFrameSubmission(std::move(submission));
}

void Renderer::SubmitUiRenderPacket(UiRenderPacket packet) noexcept
{
	m_state->Coordinator.StageUiRenderPacket(std::move(packet));
}

void Renderer::SubmitRenderingSettings(EngineRenderingSettingsState settings) noexcept
{
	m_state->Coordinator.SubmitRenderingSettings(settings);
}

void Renderer::BeginSimulationFrame(std::uint64_t frameId) noexcept
{
	m_state->ExternalRuntime.BeginSimulationFrame(frameId);
}

void Renderer::EndSimulationFrame(std::uint64_t frameId) noexcept
{
	m_state->ExternalRuntime.EndSimulationFrame(frameId);
}

ViewportCaptureAdmission Renderer::RequestViewportCapture(ViewportCaptureRequest request) noexcept
{
	return m_state->Coordinator.RequestViewportCapture(std::move(request));
}

bool Renderer::TryTakeViewportCapture(ViewportCaptureId id, ViewportCaptureReadback& readback) noexcept
{
	return m_state->Coordinator.TryTakeViewportCapture(id, readback);
}

void Renderer::OnRender() noexcept
{
	m_state->Coordinator.RenderFrame();
}

ExternalCaptureSnapshot Renderer::ObserveExternalCapture() const
{
#if SPARKLE_WITH_EXTERNAL_CAPTURE
	const RhiExternalCapture* capture = m_state->ExternalRuntime.GetDeviceLaunch().ExternalCapture;
	return capture ? capture->Observe() : ExternalCaptureSnapshot{};
#else
	return {};
#endif
}

ExternalCaptureAdmission Renderer::RequestExternalCapture(std::uint64_t viewportGeneration) noexcept
{
	return m_state->Coordinator.RequestExternalCapture(viewportGeneration);
}
