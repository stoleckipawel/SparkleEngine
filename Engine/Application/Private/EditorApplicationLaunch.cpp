#include "PCH.h"

#include "EditorApplicationLaunch.h"

#include "Application.h"
#include "EditorApplication.h"
#include "Core/Public/Threading/ThreadOwnership.h"
#include "Editor/ReferencePathTracer/ReferencePathTracerApplication.h"

int RunEditorApplication()
{
	return RunEditorApplication(RuntimeApplicationOptions{});
}

int RunEditorApplication(int argumentCount, wchar_t* arguments[])
{
	if (const std::optional<int> featureResult = TryRunReferencePathTracerApplication(argumentCount, arguments))
	{
		return *featureResult;
	}
	return RunEditorApplication();
}

int RunEditorApplication(RuntimeApplicationOptions options)
{
	Threading::SetCurrentThreadRole("Sparkle.EditorThread");
	Application::ConfigureProcessFromCommandLine();

	EditorApplication app(options);
	app.Run();
	return 0;
}
