#pragma once

#include "Frame/Presentation/OutputEncodingUniformData.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <cstdint>

EngineOutputColorEncoding SanitizeOutputColorEncoding(EngineOutputColorEncoding encoding) noexcept;
std::uint32_t ResolveShaderOutputEncoding(EngineOutputColorEncoding encoding, PixelFormat backBufferFormat) noexcept;
OutputEncodingUniformData BuildOutputEncodingUniformDataFromCVars() noexcept;
