#include "RuntimeApplicationLaunch.h"
#include "ShowcaseSceneController.h"
#include "GameFramework/Public/World/GameWorld.h"

#include <memory>

int main()
{
	return RunRuntimeApplication(
	    RuntimeApplicationOptions{
	        .WorldSetupCallback =
	            [](GameWorld& world)
	            {
		            world.RegisterController(std::make_unique<ShowcaseSceneController>());
	            }});
}
