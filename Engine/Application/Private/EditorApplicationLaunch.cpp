#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"
#include "Core/Public/Threading/ThreadOwnership.h"

int RunEditorApplication()
{
	return RunEditorApplication(RuntimeApplicationOptions{});
}

int RunEditorApplication(RuntimeApplicationOptions options)
{
	Threading::SetCurrentThreadRole("Sparkle.EditorThread");
	Application::ConfigureProcessFromCommandLine();

	EditorApplication app(std::move(options));
	app.Run();
	return 0;
}
