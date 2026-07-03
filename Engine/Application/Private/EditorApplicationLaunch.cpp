#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"

int RunEditorApplication()
{
	return RunEditorApplication(EditorApplicationOptions{});
}

int RunEditorApplication(EditorApplicationOptions options)
{
	Application::ConfigureProcessFromCommandLine();

	EditorApplication app(std::move(options));
	app.Run();
	return 0;
}
