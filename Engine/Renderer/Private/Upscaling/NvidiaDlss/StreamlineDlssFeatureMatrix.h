#pragma once

#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"

#include <string_view>

inline constexpr const char* kStreamlineDlssNotIntegratedReason =
    "NVIDIA Streamline SDK is not integrated in this build; DLSS Super Resolution remains unavailable.";

DlssFeatureMatrix CreateUnavailableStreamlineDlssFeatureMatrix(std::string_view reason);
DlssFeatureMatrix CreateStreamlineDlssFeatureMatrix(bool superResolutionSupported, std::string_view reason);
EDlssFeatureKind GetDlssFeatureForQualityMode(EUpscalerQualityMode qualityMode) noexcept;
void MarkSelectedDlssFeature(DlssFeatureMatrix& matrix, EDlssFeatureKind selectedFeature);
void MarkDlssFeatureFailed(DlssFeatureMatrix& matrix, EDlssFeatureKind feature, std::string_view reason);
