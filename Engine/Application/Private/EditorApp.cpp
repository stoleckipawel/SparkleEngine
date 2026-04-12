#include "PCH.h"
#include "EditorApp.h"

#include "ProjectApp.h"

EditorApp::EditorApp(RendererOverlayFactory overlayFactory) : m_overlayFactory(std::move(overlayFactory)) {}

EditorApp::~EditorApp() = default;

void EditorApp::Initialize()
{
	if (m_isEditorSessionActive)
	{
		return;
	}

	if (!m_projectApp)
	{
		m_projectApp = std::make_unique<ProjectApp>();
	}

	m_projectApp->Initialize();
	m_isEditorSessionActive = true;
}

bool EditorApp::Tick()
{
	if (!m_isEditorSessionActive)
	{
		return false;
	}

	return m_projectApp->Tick();
}

void EditorApp::Shutdown()
{
	if (!m_isEditorSessionActive)
	{
		return;
	}

	m_projectApp->Shutdown();
	m_isEditorSessionActive = false;
}

void EditorApp::Run()
{
	Initialize();

	while (Tick())
	{
	}

	Shutdown();
}