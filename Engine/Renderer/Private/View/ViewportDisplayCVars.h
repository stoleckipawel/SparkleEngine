#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"

extern ConsoleVariable<EngineToneMapper> CVarToneMapper;
extern ConsoleVariable<EngineExposureMode> CVarExposureMode;
extern ConsoleVariable<EngineExposureMeteringMethod> CVarExposureMeteringMethod;
extern ConsoleVariable<float> CVarManualExposure;
extern ConsoleVariable<float> CVarExposureCompensation;
extern ConsoleVariable<float> CVarExposureTargetLuminance;
extern ConsoleVariable<float> CVarExposureMin;
extern ConsoleVariable<float> CVarExposureMax;
extern ConsoleVariable<float> CVarExposureAdaptationSpeedUp;
extern ConsoleVariable<float> CVarExposureAdaptationSpeedDown;
