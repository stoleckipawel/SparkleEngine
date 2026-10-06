#include "../Private/App/TextureCookerApplication.h"

#include "Core/Public/Threading/ThreadOwnership.h"

int main(int argc, char** argv)
{
	Threading::SetCurrentThreadRole("Sparkle.ToolMain");
	return RunTextureCooker(argc, argv);
}
