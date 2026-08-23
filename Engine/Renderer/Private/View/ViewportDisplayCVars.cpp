#include "PCH.h"
#include "View/ViewportDisplayCVars.h"

ConsoleVariable<EngineToneMapper> CVarToneMapper(
    "r.ToneMapper",
    EngineToneMapper::AcesApprox,
    "Display tone mapper. 0=Reinhard, 1=ACES approximate, 2=ACES fitted filmic.");

ConsoleVariable<EngineExposureMode> CVarExposureMode(
    "r.Exposure.Mode",
    EngineExposureMode::Automatic,
    "Exposure source. 0=manual multiplier, 1=automatic scene luminance metering.");

ConsoleVariable<EngineExposureMeteringMethod> CVarExposureMeteringMethod(
    "r.Exposure.MeteringMethod",
    EngineExposureMeteringMethod::ParallelReduction,
    "Automatic exposure metering path. 0=parallel reduction, 1=downsample pyramid.");

ConsoleVariable<float> CVarManualExposure(
    "r.Exposure.Manual",
    1.0f,
    "Manual linear exposure multiplier used when r.Exposure.Mode is manual.");

ConsoleVariable<float> CVarExposureCompensation(
    "r.Exposure.Compensation",
    0.0f,
    "Exposure compensation in EV stops applied after manual or automatic exposure.");

ConsoleVariable<float> CVarExposureTargetLuminance(
    "r.Exposure.TargetLuminance",
    0.18f,
    "Automatic exposure target scene luminance after metering.");

ConsoleVariable<float> CVarExposureMin("r.Exposure.Min", 0.000001f, "Minimum linear exposure multiplier after metering and compensation.");

ConsoleVariable<float> CVarExposureMax("r.Exposure.Max", 65536.0f, "Maximum linear exposure multiplier after metering and compensation.");

ConsoleVariable<float> CVarExposureAdaptationSpeedUp(
    "r.Exposure.AdaptationSpeedUp",
    3.0f,
    "Automatic exposure adaptation speed in EV/second when target exposure increases.");

ConsoleVariable<float> CVarExposureAdaptationSpeedDown(
    "r.Exposure.AdaptationSpeedDown",
    1.0f,
    "Automatic exposure adaptation speed in EV/second when target exposure decreases.");
