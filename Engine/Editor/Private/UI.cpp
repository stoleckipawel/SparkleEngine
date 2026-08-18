#include "PCH.h"

#include "UI.h"

#include "Console/EditorConsoleSystem.h"
#include "Panels/MainMenuBarPanel.h"
#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/UsedMeshesPanel.h"
#include "Panels/UsedShadersPanel.h"
#include "Panels/UsedTexturesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ViewportTopPanel.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Renderer/Public/UI/ImGuiRenderPacketBuilder.h"
#include "Scene/Model/EditorSceneModel.h"
#include "Scene/Model/EditorSceneModelBuilder.h"
#include "Scene/Transactions/EditorTransactionHistory.h"
#include "Settings/EditorRestartService.h"
#include "Viewport/EditorViewportSession.h"
#include "Window/Window.h"

#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <utility>

const ViewportRenderRequest& UI::GetViewportRenderRequest() const noexcept
{
	static const ViewportRenderRequest defaultRequest = []
	{
		ViewportRenderRequest request{};
		request.ViewportId = 1;
		request.ViewKind = RenderViewKind::Game;
		request.RequestedOutputs = RenderOutputFlags::SceneColor | RenderOutputFlags::SceneDepth;
		return request;
	}();

	return m_viewportPanel ? m_viewportPanel->GetRenderRequest() : defaultRequest;
}

RenderViewCameraData UI::UpdateViewportCamera(const CameraInputIntent& intent, float deltaSeconds) noexcept
{
	UpdateSceneModel();
	if (!m_viewportSession || !m_sceneModel)
	{
		return {};
	}

	const RenderViewportExtent extent = GetViewportRenderRequest().Extent;
	m_viewportSession->SynchronizeWorld(m_sceneModel->GetCameras(), m_sceneModel->GetWorldGeneration());
	const RenderViewCameraData camera = m_viewportSession->UpdateCamera(intent, deltaSeconds, extent);
	if (m_viewportPanel)
	{
		m_viewportPanel->SetExposureOverrides(m_viewportSession->GetSettings().Exposure);
	}
	return camera;
}

void UI::SetViewportRenderProducts(const ViewportRenderProducts& products) noexcept
{
	m_viewportGeneration = products.GetGeneration();
	if (m_viewportPanel)
	{
		m_viewportPanel->SetRenderProducts(products);
	}
}

void UI::SetViewportSceneColorTexture(EditorTextureHandle texture) noexcept
{
	if (m_viewportPanel)
	{
		m_viewportPanel->SetSceneColorTexture(texture);
	}
}

void UI::SetDiagnosticsProviders(EditorDiagnosticsProviders providers)
{
	m_shaderPackageGenerationProvider = std::move(providers.ShaderPackageGeneration);
	m_meshDiagnosticsProvider = std::move(providers.MeshDiagnostics);
	m_textureDiagnosticsProvider = std::move(providers.TextureDiagnostics);
	m_memoryDiagnosticsProvider = std::move(providers.MemoryDiagnostics);
	m_meshPreviewProvider = std::move(providers.MeshPreview);

	if (m_usedShadersPanel)
	{
		m_usedShadersPanel->SetGenerationProvider(m_shaderPackageGenerationProvider);
	}
	if (m_usedMeshesPanel)
	{
		m_usedMeshesPanel->SetDiagnosticsProvider(m_meshDiagnosticsProvider);
		m_usedMeshesPanel->SetPreviewGeometryProvider(m_meshPreviewProvider);
	}
	if (m_usedTexturesPanel)
	{
		m_usedTexturesPanel->SetDiagnosticsProvider(m_textureDiagnosticsProvider);
	}
	ConfigureMainMenuBarWindowActions();
}

RendererMemoryDiagnosticsSnapshot UI::CaptureMemoryDiagnostics() const
{
	return m_memoryDiagnosticsProvider ? m_memoryDiagnosticsProvider() : RendererMemoryDiagnosticsSnapshot{};
}

bool UI::ConsumeShaderReloadRequest() noexcept
{
	const bool requested = m_shaderReloadRequested;
	m_shaderReloadRequested = false;
	return requested;
}

bool UI::ConsumeShaderRecookRequest() noexcept
{
	const bool requested = m_shaderRecookRequested;
	m_shaderRecookRequested = false;
	return requested;
}

bool UI::ConsumeViewportCaptureRequest() noexcept
{
	const bool requested = m_viewportCaptureRequested;
	m_viewportCaptureRequested = false;
	return requested;
}

UiRenderPacket UI::ConsumeRenderPacket()
{
	return std::move(m_renderPacket);
}

UI::UI(EditorHostServices hostServices) :
    m_timer(&hostServices.RuntimeTimer),
    m_levelSession(hostServices.Levels),
    m_window(&hostServices.HostWindow),
    m_inputSystem(&hostServices.Input),
    m_sceneSelection(SceneObjectSelection::None())
{
	m_sceneModelBuilder = std::make_unique<EditorSceneModelBuilder>(EditorSceneSource{
	    .AcquireReadView = std::move(hostServices.AcquireWorldReadView),
	    .ReadChanges = std::move(hostServices.ReadWorldChanges),
	    .AcknowledgeChanges = std::move(hostServices.AcknowledgeWorldChanges),
	    .WorldGeneration = std::move(hostServices.WorldGeneration),
	    .MaterialVariants = std::move(hostServices.MaterialVariants)});
	m_transactionHistory = std::make_unique<EditorTransactionHistory>(std::move(hostServices.SubmitWorldEdit));
	m_renderPacketBuilder = std::make_unique<ImGuiRenderPacketBuilder>();

	InitializeImGuiContext();
	ApplyDpiScale(m_window->GetDpiScale());
	if (!InitializeWin32Backend())
	{
		return;
	}

	InitializeDefaultPanels();
	if (m_renderingSettings)
	{
		m_renderingSettings->SetCommitHandler(std::move(hostServices.SubmitRenderingSettings));
	}
	SubscribeToWindowEvents(hostServices.HostWindow);
}

UI::~UI() noexcept
{
	m_windowDpiScaleHandle.Reset();
	m_windowMessageHandle.Reset();

	if (m_isWin32BackendInitialized)
	{
		ImGui_ImplWin32_Shutdown();
		m_isWin32BackendInitialized = false;
	}
	if (m_isImGuiContextInitialized)
	{
		ImGui::DestroyContext();
		m_isImGuiContextInitialized = false;
	}
}

void UI::Update()
{
	if (!IsReady())
	{
		return;
	}

	NewFrame();
	Build();
	m_renderPacket = m_renderPacketBuilder->Build(*ImGui::GetDrawData(), UiPresentationMode::EditorViewport, m_viewportGeneration);
}

bool UI::IsReady() const noexcept
{
	return m_isImGuiContextInitialized && m_isWin32BackendInitialized;
}
