#include "PCH.h"
#include "Renderer.h"

#include "FramePipeline/FramePipeline.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Core/Public/Threading/ThreadOwnership.h"

class RendererState final
{
  public:
	RendererState(Timer& timer, Window& window) noexcept
	{
		m_systems = std::make_unique<RendererSystemRoot>(timer, window);
		m_pipeline = std::make_unique<FramePipeline>(*m_systems);
		m_systems->PostLoad();
	}

	~RendererState() noexcept { m_owner.AssertAccess(); }

	RendererSystemRoot& Systems(std::source_location location = std::source_location::current()) noexcept
	{
		m_owner.AssertAccess(location);
		return *m_systems;
	}

	const RendererSystemRoot& Systems(std::source_location location = std::source_location::current()) const noexcept
	{
		m_owner.AssertAccess(location);
		return *m_systems;
	}

	FramePipeline& Pipeline(std::source_location location = std::source_location::current()) noexcept
	{
		m_owner.AssertAccess(location);
		return *m_pipeline;
	}

	const FramePipeline& Pipeline(std::source_location location = std::source_location::current()) const noexcept
	{
		m_owner.AssertAccess(location);
		return *m_pipeline;
	}

  private:
	Threading::OwnerThread m_owner{"Renderer"};
	std::unique_ptr<RendererSystemRoot> m_systems;
	std::unique_ptr<FramePipeline> m_pipeline;
};

Renderer::Renderer(Timer& timer, Window& window) noexcept
{
	m_state = std::make_unique<RendererState>(timer, window);
}

Renderer::~Renderer() noexcept = default;

void Renderer::SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept
{
	m_state->Pipeline().SubmitViewportRenderRequest(request);
}

const ViewportRenderProducts& Renderer::GetViewportRenderProducts() const noexcept
{
	return m_state->Pipeline().GetViewportRenderProducts();
}

RhiImGuiRenderer& Renderer::GetImGuiRenderer() noexcept
{
	return m_state->Systems().GetImGuiRenderer();
}

CookedShaderReloadResult Renderer::ReloadCookedShaders() noexcept
{
	return m_state->Systems().ReloadCookedShaders();
}

std::uint64_t Renderer::GetShaderPackageGeneration() const noexcept
{
	return m_state->Systems().GetShaderPackageGeneration();
}

MeshDiagnosticsSnapshot Renderer::CaptureMeshDiagnostics() const
{
	return m_state->Systems().CaptureMeshDiagnostics();
}

TextureDiagnosticsSnapshot Renderer::CaptureTextureDiagnostics() const
{
	return m_state->Systems().CaptureTextureDiagnostics();
}

RendererMemoryDiagnosticsSnapshot Renderer::CaptureMemoryDiagnostics() const
{
	return m_state->Systems().CaptureMemoryDiagnostics();
}

void Renderer::PrepareHostFrame() noexcept
{
	m_state->Pipeline().PrepareHostFrame();
}

void Renderer::RecordHostFrame() noexcept
{
	m_state->Pipeline().RecordHostFrame();
}

void Renderer::SubmitHostFrame() noexcept
{
	m_state->Pipeline().SubmitHostFrame();
}

MeshPreviewGeometry Renderer::CaptureMeshPreview(std::uintptr_t meshRuntimeId) const
{
	return m_state->Systems().CaptureMeshPreview(meshRuntimeId);
}

void Renderer::SubmitRenderInput(RenderInputFrame input) noexcept
{
	m_state->Pipeline().SubmitRenderInput(std::move(input));
}

void Renderer::WaitForIdle() noexcept
{
	m_state->Systems().GetBackend().WaitForIdle();
}

void Renderer::BeginHostPresentation(const float clearColor[4]) noexcept
{
	m_state->Systems().GetRenderHardwareInterface().GetPresentationService().BeginPresentRenderPass(clearColor);
}

void Renderer::BeginHostOverlayPresentation() noexcept
{
	m_state->Systems().GetRenderHardwareInterface().GetPresentationService().BeginPresentOverlayPass();
}

void Renderer::EndHostPresentation() noexcept
{
	m_state->Systems().GetRenderHardwareInterface().GetPresentationService().EndPresentRenderPass();
}

ViewportPresentationProduct Renderer::BeginViewportPresentation(RenderOutputFlags output) noexcept
{
	return m_state->Pipeline().BeginViewportPresentation(output);
}

void Renderer::EndViewportPresentation(RenderOutputFlags output) noexcept
{
	m_state->Pipeline().EndViewportPresentation(output);
}

ViewportCaptureResult Renderer::CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept
{
	return m_state->Pipeline().CaptureViewportProductToBmp(request);
}

void Renderer::OnRender() noexcept
{
	m_state->Pipeline().OnRender();
}
