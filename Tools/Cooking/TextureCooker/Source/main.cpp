#include "../Private/App/TextureCookerApplication.h"

#include "Core/Public/Threading/ThreadOwnership.h"

int main(int argc, char** argv)
{
	Threading::SetCurrentThreadRole("Sparkle.ToolMain");
	const TextureCookerApplication application;
	return application.Run(argc, argv);
}
