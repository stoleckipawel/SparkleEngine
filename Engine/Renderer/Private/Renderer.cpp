#include "PCH.h"
#include "Renderer.h"

#include "Concurrency/Coordinator/RenderCoordinator.h"
#include "Integrations/RendererExternalRuntime.h"

class RendererFacadeState final
{
  public:
	RendererFacadeState(Timer& timer, Window& window, RendererExecutionConfig config) :
	    Coordinator(timer, window, config, ExternalRuntime.GetBackendConfiguration())
	{
	}

	RendererExternalRuntime ExternalRuntime;
	RenderCoordinator Coordinator;
};

Renderer::Renderer(Timer& timer, Window& window, RendererExecutionConfig config) noexcept :
    m_state(std::make_unique<RendererFacadeState>(timer, window, config))
{
}

Renderer::~Renderer() noexcept = default;

void Renderer::SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept
{
	m_state->Coordinator.SubmitViewportRequest(request);
}

ViewportRenderProducts Renderer::GetViewportRenderProducts() const
{
	return m_state->Coordinator.GetViewportRenderProducts();
}

void Renderer::ReloadCookedShaders()
{
	m_state->Coordinator.ReloadCookedShaders();
}

std::uint64_t Renderer::GetShaderPackageGeneration() const noexcept
{
	return m_state->Coordinator.GetShaderPackageGeneration();
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

void Renderer::SubmitRenderInput(RenderInputFrame input) noexcept
{
	m_state->Coordinator.StageRenderInput(std::move(input));
}

void Renderer::SubmitUiRenderPacket(UiRenderPacket packet) noexcept
{
	m_state->Coordinator.StageUiRenderPacket(std::move(packet));
}

void Renderer::SubmitRenderingSettings(EngineRenderingSettingsState settings) noexcept
{
	m_state->Coordinator.SubmitRenderingSettings(std::move(settings));
}

void Renderer::BeginSimulationFrame(std::uint64_t frameId) noexcept
{
	m_state->ExternalRuntime.BeginSimulationFrame(frameId);
}

void Renderer::EndSimulationFrame(std::uint64_t frameId) noexcept
{
	m_state->ExternalRuntime.EndSimulationFrame(frameId);
}

ViewportCaptureId Renderer::RequestViewportCapture(
    ViewportCaptureRequest request) noexcept
{
	return m_state->Coordinator.RequestViewportCapture(std::move(request));
}

bool Renderer::TryTakeViewportCapture(
    ViewportCaptureReadback& readback) noexcept
{
	return m_state->Coordinator.TryTakeViewportCapture(readback);
}

void Renderer::OnRender() noexcept
{
	m_state->Coordinator.RenderFrame();
}
