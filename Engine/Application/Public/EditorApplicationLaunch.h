#pragma once

#include "ApplicationAPI.h"
#include "EditorApplication.h"

SPARKLE_APPLICATION_API int RunEditorApplication();
SPARKLE_APPLICATION_API int RunEditorApplication(int argumentCount, wchar_t* arguments[]);
SPARKLE_APPLICATION_API int RunEditorApplication(RuntimeApplicationOptions options);
