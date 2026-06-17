#include "RuntimeApplicationLaunch.h"
#include "ShowcaseSceneController.h"

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
