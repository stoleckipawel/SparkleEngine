#pragma once

#include "Core/RhiCapabilities.h"

#include <string>

std::string FormatBackendVersionInfo(const RhiBackendVersionInfo& version);
std::string FormatBackendDiagnosticsSupport(const RhiBackendDiagnosticsSupport& diagnostics);
std::string FormatBackendMemorySupport(const RhiBackendMemorySupport& memory);
std::string FormatExternalFeatureInteropCapabilities(const RhiExternalFeatureInteropCapabilities& capabilities);
