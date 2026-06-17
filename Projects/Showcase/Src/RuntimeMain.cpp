#include "RuntimeApplicationLaunch.h"
#include "ShowcaseSceneController.h"
#include "GameFramework/Public/Scene/GameScene.h"

#include <memory>

int main()
{
	return RunRuntimeApplication(
	    RuntimeApplicationOptions{
	        .SceneSetupCallback =
	            [](GameScene& scene)
	            {
		            scene.RegisterController(std::make_unique<ShowcaseSceneController>());
	            }});
}
