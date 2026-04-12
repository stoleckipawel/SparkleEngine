#include "PCH.h"
#include "App.h"

#include "ProjectApp.h"

App::App() : m_projectApp(std::make_unique<ProjectApp>()) {}

App::~App() = default;

void App::Run()
{
	m_projectApp->Run();
}