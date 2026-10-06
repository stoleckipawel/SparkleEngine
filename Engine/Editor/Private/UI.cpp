#include "PCH.h"
#include "UI.h"
#include "UIImplementation.h"

#include <utility>

UI::UI(EditorHostServices hostServices) :
    m_implementation(std::make_unique<Implementation>(std::move(hostServices)))
{
}
UI::~UI() noexcept = default;

const ViewportRenderRequest& UI::GetViewportRenderRequest() const noexcept
{
	return m_implementation->GetViewportRenderRequest();
}

RenderViewCameraData UI::UpdateViewportCamera(const CameraInputIntent& intent, float deltaSeconds) noexcept
{
	return m_implementation->UpdateViewportCamera(intent, deltaSeconds);
}

void UI::SetViewportRenderProducts(const ViewportRenderProducts& products) noexcept
{
	m_implementation->SetViewportRenderProducts(products);
}

void UI::SetViewportFinalColorTexture(UiTextureHandle texture) noexcept
{
	m_implementation->SetViewportFinalColorTexture(texture);
}

void UI::SetDiagnosticsProviders(EditorDiagnosticsProviders providers)
{
	m_implementation->SetDiagnosticsProviders(std::move(providers));
}

RendererMemoryDiagnosticsSnapshot UI::CaptureMemoryDiagnostics() const
{
	return m_implementation->CaptureMemoryDiagnostics();
}

EditorConsoleSystem* UI::GetEditorConsoleSystem() noexcept
{
	return m_implementation->GetEditorConsoleSystem();
}

bool UI::ConsumeShaderReloadRequest() noexcept
{
	return m_implementation->ConsumeShaderReloadRequest();
}

bool UI::ConsumeShaderRecookRequest() noexcept
{
	return m_implementation->ConsumeShaderRecookRequest();
}

ViewportOutputAction UI::ConsumeViewportOutputAction() noexcept
{
	return m_implementation->ConsumeViewportOutputAction();
}

UiRenderPacket UI::ConsumeRenderPacket()
{
	return m_implementation->ConsumeRenderPacket();
}

void UI::Update()
{
	m_implementation->Update();
}
