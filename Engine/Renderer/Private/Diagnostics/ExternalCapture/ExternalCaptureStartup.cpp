#include "PCH.h"
#include "Renderer/Public/ExternalCapture/ExternalCaptureStartup.h"
ConsoleVariable<ExternalCaptureProvider> CVarExternalCaptureStartupProvider(
    "r.ExternalCapture.StartupProvider",
    ExternalCaptureProvider::None,
    "Capture tool on Editor startup: 0=None, 1=Nsight Graphics, 2=PIX, 3=RenderDoc. Restart required.");
