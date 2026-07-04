#include "PCH.h"
#include "Renderer.h"

#include "FramePipeline/FramePipeline.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

Renderer::Renderer(Timer& timer, GameScene& gameScene, Window& window, LevelManager& levelManager) noexcept
{
	m_systemRoot = std::make_unique<RendererSystemRoot>(timer, gameScene, window, levelManager);
	m_framePipeline = std::make_unique<FramePipeline>(*m_systemRoot);
	m_systemRoot->PostLoad();
}

Renderer::~Renderer() noexcept = default;

void Renderer::SubmitViewportRenderRequest(const ViewportRenderRequest& request) noexcept
{
	m_framePipeline->SubmitViewportRenderRequest(request);
}

const ViewportRenderProducts& Renderer::GetViewportRenderProducts() const noexcept
{
	return m_framePipeline->GetViewportRenderProducts();
}

RenderHardwareInterface& Renderer::GetRenderHardwareInterface() noexcept
{
	return m_systemRoot->GetRenderHardwareInterface();
}

const RenderHardwareInterface& Renderer::GetRenderHardwareInterface() const noexcept
{
	return m_systemRoot->GetRenderHardwareInterface();
}

RhiCommandSubmissionService& Renderer::GetCommandSubmissionService() noexcept
{
	return m_systemRoot->GetBackend();
}

RhiImGuiRenderer& Renderer::GetImGuiRenderer() noexcept
{
	return m_systemRoot->GetImGuiRenderer();
}

CookedShaderReloadResult Renderer::ReloadCookedShaders() noexcept
{
	return m_systemRoot->ReloadCookedShaders();
}

std::uint64_t Renderer::GetShaderPackageGeneration() const noexcept
{
	return m_systemRoot->GetShaderPackageGeneration();
}

MeshDiagnosticsSnapshot Renderer::CaptureMeshDiagnostics() const
{
	return m_systemRoot->CaptureMeshDiagnostics();
}

TextureDiagnosticsSnapshot Renderer::CaptureTextureDiagnostics() const
{
	return m_systemRoot->CaptureTextureDiagnostics();
}

RendererMemoryDiagnosticsSnapshot Renderer::CaptureMemoryDiagnostics() const
{
	return m_systemRoot->CaptureMemoryDiagnostics();
}

void Renderer::PrepareHostFrame() noexcept
{
	m_framePipeline->PrepareHostFrame();
}

void Renderer::RecordHostFrame() noexcept
{
	m_framePipeline->RecordHostFrame();
}

void Renderer::SubmitHostFrame() noexcept
{
	m_framePipeline->SubmitHostFrame();
}

ViewportPresentationProduct Renderer::BeginViewportPresentation(RenderOutputFlags output) noexcept
{
	return m_framePipeline->BeginViewportPresentation(output);
}

void Renderer::EndViewportPresentation(RenderOutputFlags output) noexcept
{
	m_framePipeline->EndViewportPresentation(output);
}

ViewportCaptureResult Renderer::CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept
{
	return m_framePipeline->CaptureViewportProductToBmp(request);
}

void Renderer::OnRender() noexcept
{
	m_framePipeline->OnRender();
}
