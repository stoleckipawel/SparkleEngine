#include "EditorApplicationLaunch.h"

int main()
{
	return RunEditorApplication(
	    RuntimeApplicationOptions{
	        .EnableOscillatingMeshMotion = true});
}
