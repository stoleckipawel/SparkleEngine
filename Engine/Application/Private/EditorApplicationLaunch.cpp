#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"

int RunEditorApplication()
{
	return RunEditorApplication(RuntimeApplicationOptions{});
}

int RunEditorApplication(RuntimeApplicationOptions options)
{
	Application::ConfigureProcessFromCommandLine();

	EditorApplication app(std::move(options));
	app.Run();
	return 0;
}
