#include "RuntimeApplicationLaunch.h"
#include "GameFramework/Public/World/GameWorld.h"

int main()
{
	return RunRuntimeApplication(
	    RuntimeApplicationOptions{
	        .WorldSetupCallback =
	            [](GameWorld& world)
		            { world.EnableOscillatingMeshMotion(); }});
}
