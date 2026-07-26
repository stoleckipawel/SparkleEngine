#include "RuntimeApplicationLaunch.h"

int main()
{
	return RunRuntimeApplication(
	    RuntimeApplicationOptions{
	        .EnableOscillatingMeshMotion = true});
}
