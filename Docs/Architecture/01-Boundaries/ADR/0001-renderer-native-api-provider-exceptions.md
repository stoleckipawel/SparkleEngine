# ADR 0001: Renderer Native API Provider Exceptions

## Context

SparkleEngine enforces `RENDERER_NO_BACKEND_NATIVE` so that general renderer code does not depend directly on D3D12 or Vulkan native APIs. This keeps backend-native ownership inside `RHI` and prevents the renderer from becoming a second backend layer.

At the same time, current NVIDIA DLSS/Streamline integration still requires a narrow bridge in provider-scoped renderer code:

- `Engine/Renderer/CMakeLists.txt`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.cpp`

The architecture boundary check therefore contains two counted exceptions:

- `RENDERER_NO_BACKEND_NATIVE: NVIDIA DLSS provider Vulkan::Vulkan link`
- `RENDERER_NO_BACKEND_NATIVE: Streamline DLSS Vulkan bridge`

These exceptions are file-scoped, pattern-scoped, and frozen by count.

## Decision

Allow the existing NVIDIA provider exceptions to remain temporarily, but only under these conditions:

- the exception remains provider-scoped
- the exception remains counted
- the exception remains limited to the currently named files and patterns
- the exception is not interpreted as general renderer permission for native API usage

This is a temporary architectural seam for provider bridging, not a change in renderer ownership policy.

## Current Counted Exception Location

Counted exception 1:

- File: `Engine/Renderer/CMakeLists.txt`
- Allowed pattern scope:
  - `if(TARGET Vulkan::Vulkan)`
  - `target_link_libraries(SparkleRendererNvidiaDlssProvider PRIVATE Vulkan::Vulkan)`
- Frozen baseline count: `2`

Counted exception 2:

- File: `Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.cpp`
- Allowed pattern scope:
  - `#include <vulkan/vulkan.h>`
  - `vulkanInfo.instance = static_cast<VkInstance>(...)`
  - `vulkanInfo.physicalDevice = static_cast<VkPhysicalDevice>(...)`
  - `vulkanInfo.device = static_cast<VkDevice>(...)`
  - `adapterInfo.vkPhysicalDevice = ...`
- Frozen baseline count: `5`

## Why The Exception Is Allowed

- Streamline/DLSS integration currently needs a narrow Vulkan bridge in provider code.
- The bridge is isolated in a dedicated provider target rather than spread through general renderer files.
- The count freeze makes growth visible and prevents quiet architecture drift.
- This lets the engine continue to use the provider while keeping pressure on the codebase to avoid normalizing renderer-native Vulkan access.

## Risk

- Future contributors may misread the provider bridge as permission to add more native API use in renderer code.
- Provider-specific integration may become sticky if the retirement path is never revisited.
- A provider seam living in renderer space can still create architectural drag if the RHI/provider contract is not strengthened.

## Retirement Path

The long-term goal is to reduce or eliminate renderer-native provider bridging by moving toward a stronger backend-neutral provider contract and explicit interop surface.

Retirement should happen through one or more of these paths:

- move required native-handle handoff behind a cleaner RHI/provider interop contract
- reduce the number of native Vulkan touchpoints in provider runtime code
- shrink the counted baseline as implementation details are pushed downward or abstracted better
- remove the exception entirely if provider integration no longer needs renderer-side native API access

## Acceptance Criteria For Removing Or Tightening The Exception

- Any reduction in counted matches should trigger review of whether the exception can be tightened or removed.
- No new files may be added to this exception without a new or updated ADR.
- The counted baseline must not grow above the frozen value without an explicit architectural decision.
- Future provider work must prefer backend-neutral contracts before requesting new native API exceptions.
- Renderer-wide native API access remains prohibited even while this exception exists.

