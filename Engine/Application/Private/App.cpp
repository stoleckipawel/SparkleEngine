#include "PCH.h"
#include "App.h"

#include "ProjectApp.h"

App::App(RendererOverlayFactory overlayFactory) : m_projectApp(std::make_unique<ProjectApp>(std::move(overlayFactory))) {}

App::~App() = default;

void App::Run()
{
	m_projectApp->Run();
}