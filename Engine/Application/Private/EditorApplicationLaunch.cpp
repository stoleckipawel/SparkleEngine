#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"
#include "Core/Public/Threading/ThreadOwnership.h"

int RunEditorApplication()
{
	Threading::SetCurrentThreadRole("Sparkle.EditorThread");
	Application::ConfigureProcessFromCommandLine();

	EditorApplication app;
	app.Run();
	return 0;
}
