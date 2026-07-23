#pragma once

// Same-thread composition hook retained for the serial editor/runtime-console
// path. No pointer or callback is stored or published to RenderThread.
using RendererSerialUiCallback = void (*)(void* context) noexcept;
