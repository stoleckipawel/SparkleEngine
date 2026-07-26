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

RhiImGuiRenderer& Renderer::GetImGuiRenderer() noexcept
{
	return m_state->Coordinator.GetSerialImGuiRenderer();
}

CookedShaderReloadResult Renderer::ReloadCookedShaders() noexcept
{
	return m_state->Coordinator.ReloadCookedShaders();
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

void Renderer::RenderSerialUiFrame(RendererSerialUiCallback composeUi, void* context) noexcept
{
	m_state->Coordinator.RenderSerialUiFrame(composeUi, context);
}

MeshPreviewGeometry Renderer::CaptureMeshPreview(std::uintptr_t meshRuntimeId) const
{
	return m_state->Coordinator.CaptureMeshPreview(meshRuntimeId);
}

void Renderer::SubmitRenderInput(RenderInputFrame input) noexcept
{
	m_state->Coordinator.StageRenderInput(std::move(input));
}

void Renderer::SubmitEditorRenderPacket(EditorRenderPacket packet) noexcept
{
	m_state->Coordinator.StageEditorRenderPacket(std::move(packet));
}

void Renderer::SubmitRenderingSettings(EngineRenderingSettingsState settings) noexcept
{
	m_state->Coordinator.SubmitRenderingSettings(std::move(settings));
}

void Renderer::BeginHostPresentation(const float clearColor[4]) noexcept
{
	m_state->Coordinator.BeginSerialHostPresentation(clearColor);
}

void Renderer::BeginHostOverlayPresentation() noexcept
{
	m_state->Coordinator.BeginSerialHostOverlayPresentation();
}

void Renderer::EndHostPresentation() noexcept
{
	m_state->Coordinator.EndSerialHostPresentation();
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
