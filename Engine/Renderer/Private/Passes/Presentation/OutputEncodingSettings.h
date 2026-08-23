#pragma once

#include "ShaderData/OutputEncodingUniformData.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <cstdint>

std::uint32_t ResolveShaderOutputEncoding(EngineOutputColorEncoding encoding, PixelFormat backBufferFormat) noexcept;
OutputEncodingUniformData BuildOutputEncodingUniformData() noexcept;
