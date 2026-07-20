#include "EditorApplicationLaunch.h"
#include "GameFramework/Public/World/GameWorld.h"

int main()
{
	return RunEditorApplication(
	    RuntimeApplicationOptions{
	        .WorldSetupCallback =
	            [](GameWorld& world)
		            { world.EnableOscillatingMeshMotion(); }});
}
