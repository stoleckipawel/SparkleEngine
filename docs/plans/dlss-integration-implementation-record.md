# DLSS Integration Implementation Record

Date: 2026-06-11

Roadmap phase: Stage 8.2.1, Reference Study And Integration Decision

Status: Proposed integration shape. Documentation-only phase.

## References Reviewed

- NVIDIA Streamline programming guide, `NVIDIA-RTX/Streamline`, `main` at `019994e18d256a3e92347888deb527feb7f58bc0`, Streamline SDK 2.11.1, accessed 2026-06-11.
- NVIDIA Streamline DLSS programming guide, `NVIDIA-RTX/Streamline`, `main` at `019994e18d256a3e92347888deb527feb7f58bc0`, Streamline SDK 2.11.1, accessed 2026-06-11.
- NVIDIA Streamline Sample, `NVIDIA-RTX/Streamline_Sample`, `main` at `e66c6d7fb967234bb96138a26d6ccfc3ba37acba`, accessed 2026-06-11.
- NVIDIA Vulkan Streamline sample, `nvpro-samples/vk_streamline`, `main` at `02fe6f70dfc9ce809cb8bca312fe26c8a69a3cd9`, accessed 2026-06-11.
- NVIDIA DLSS programming guide release PDF, `NVIDIA/DLSS`, `main` at `d1bef2006b41eefd9d44b0a05f123993f3acbf3c`, accessed 2026-06-11.

## Integration Decision

Sparkle should integrate production DLSS through NVIDIA Streamline first, not through direct NGX as the primary path.

Reasons:

- Streamline is NVIDIA's current integration layer for DLSS features and exposes the production shape Sparkle needs: feature loading, device setup, capability queries, resource tagging, per-frame constants, and feature evaluation.
- The API model matches the architecture Sparkle wants to grow into: renderer-owned feature lifetime, RHI-owned native handle access, FrameGraph-owned resource declaration, and provider-owned execution.
- Streamline supports D3D12 and Vulkan integration paths. The `vk_streamline` sample documents both automatic interposer and manual Vulkan hooking paths, which gives Sparkle room to choose the lower-surprise backend integration after the RHI bridge is designed.
- Keeping DLSS behind a Streamline-backed upscaler provider avoids leaking NVIDIA-specific concepts into general frame orchestration and leaves room for other temporal upscalers later.

Direct NGX should remain a fallback investigation path only if Streamline blocks a required backend, packaging model, or feature requirement that Sparkle cannot satisfy.

## Practices To Inherit

The implementation should use NVIDIA's integration model as evidence for mature renderer architecture, not as a reason to leak SDK concepts across Sparkle. Sparkle should inherit the practices that make production integrations reliable:

- External features are provider-owned runtime systems, not ordinary authored shader passes.
- Vendor SDK calls are isolated behind narrow adapters.
- Frame orchestration schedules declared work but does not know SDK implementation details.
- Resource contracts are explicit: each provider declares required inputs, outputs, states, extents, and fallback behavior.
- RHI exposes native handles and capability facts without owning vendor policy.
- Feature availability is a runtime capability result with diagnostics, not a compile-time assumption.
- Presentation consumes a renderer-owned final product and does not branch on provider type.

## Reference API Pressure On Sparkle Architecture

| Reference API pressure | Sparkle architectural response |
| --- | --- |
| `slInit`, preferences, requested feature list, log callback | Future renderer bootstrapping hook for a `SuperResolution` or `Upscaler` subsystem. This must initialize early enough to satisfy Streamline's graphics API interception requirements. |
| `slShutdown` | Renderer shutdown path after Streamline features are freed and before graphics objects are destroyed. |
| `slSetD3DDevice` / `slSetVulkanInfo` | RHI native bridge using `RenderHardwareInterface::GetBackendApi`, `GetDeviceHandle`, `GetGraphicsQueueHandle`, and command-list native handles. |
| `slIsFeatureSupported` / `slGetFeatureRequirements` | Renderer-facing upscaler capability query backed by RHI backend/device details, not by renderer-side hard-coded API checks. |
| `slDLSSGetOptimalSettings` | Viewport/render-extent policy. Sparkle should ask DLSS for render dimensions per quality mode and treat that as part of the renderer resolution contract. |
| `slSetTagForFrame` | FrameGraph external resource tagging for color input, color output, depth, motion vectors, and optional exposure. Tags must be declared near the provider pass, not scattered through general frame orchestration. |
| `slDLSSSetOptions` | Provider-owned DLSS settings translated from renderer config/UI: quality mode, output extent, sharpness, HDR mode, exposure mode, and reset state. |
| `slSetConstants` | Per-view temporal contract: jittered and non-jittered matrices, camera jitter, motion-vector scale, frame index, camera state, reset/history flags, render extent, and output extent. |
| `slEvaluateFeature` | FrameGraph external provider execution after scene color is produced and before UI/compositing/presentation consumes the final output. The host must restore command-list state expected by subsequent Sparkle passes. |
| Multiple viewport support | Future multi-view contract. Stage 8.2 should start with one viewport but avoid global singletons that prevent additional view IDs later. |

## Required DLSS Inputs

| DLSS input | Current Sparkle source | Required work |
| --- | --- | --- |
| Color input | `SceneRenderTargets::SceneColor` after main scene rendering | Ensure it is HUD-less and in the expected HDR/color-space state before tagging. |
| Color output | No dedicated upscaled final product yet | Add a FrameGraph-owned final scene color product so presentation does not need to know whether DLSS ran. |
| Depth | `SceneRenderTargets::MainDepth` / G-Buffer device depth | Confirm depth convention, format, state transition, and whether DLSS receives device Z or a linearized depth resource. |
| Motion vectors | `GBufferRenderTargets::MotionVector` | Sparkle currently emits pixel-space vectors. Formalize sign, origin, viewport scale, and provide the Streamline motion-vector scale constants. |
| Jitter | `RenderViewData::temporalState` and temporal constant buffers | Ensure matrices passed to Streamline follow the required jitter/no-jitter convention. |
| Exposure | No stable renderer-owned exposure texture contract yet | Start with Streamline auto-exposure only if allowed by chosen settings, then add explicit exposure once the renderer exposure path exists. |
| Reset/history validity | `TemporalDataBuilder` reset state and `RenderTemporalFrameState::HistoryValid` | Map deterministic resize, level switch, and camera-cut resets into DLSS option updates. |
| Render/output extents | Renderer viewport extent and RHI viewport/scissor state | Add an explicit render extent versus output extent contract before dynamic resolution is enabled. |
| Frame token / viewport ID | Frame runtime state | Add provider-owned stable IDs; do not rely on global implicit frame state. |

## Required Per-Frame Constants

- Current and previous view/projection matrices in the convention required by Streamline, including any required no-jitter variants.
- Current and previous jitter offsets.
- Motion-vector scale and sign convention.
- Render extent and output/display extent.
- Camera position, camera direction, near/far plane, field of view, and frame index where required by the selected Streamline constants structure.
- History/reset state for resize, scene cut, quality-mode change, DLSS toggle, and invalid history.
- Viewport ID and frame token for the evaluated view.

## Ownership Decisions

- Renderer owns the upscaler subsystem, provider selection, DLSS feature lifetime, quality mode, reset propagation, diagnostics, and fallback policy.
- RHI owns native device, queue, command-list, resource, and backend capability exposure. RHI should not own DLSS policy or Streamline feature lifetime.
- FrameGraph owns pass ordering, resource declarations, resource lifetime, resource state intent, and the final scene color product consumed by later passes.
- A dedicated external-provider pass adapter should own Streamline tagging and evaluation. General frame orchestration should only schedule the provider pass through a narrow interface.
- Runtime PSO handling remains for Sparkle-authored raster/compute/ray-tracing passes. DLSS execution is an external SDK dispatch path and should not be represented as a normal Sparkle shader PSO.
- Presentation consumes the final renderer product and should not branch on DLSS, Streamline, NGX, or quality mode.

## Separation Rules For The Changelist

- Do not add DLSS, Streamline, NGX, or NVIDIA-specific fields to `FrameContext`, `RenderViewData`, general frame target structs, lighting passes, denoiser passes, or presentation.
- General frame code may carry provider-neutral contracts only: temporal state, motion-vector availability, render extent, output extent, final scene color, and diagnostics.
- DLSS-specific names, SDK enums, SDK handles, resource tags, option structs, and binary-loading policy belong only inside the NVIDIA provider module and its narrow platform/RHI bridge.
- If general orchestration needs a new concept, name it by renderer responsibility, such as `ExternalFeature`, `Upscaler`, `FinalSceneColor`, `RenderExtent`, `OutputExtent`, or `ProviderEvaluation`, not by the first provider that uses it.
- FrameGraph additions must be reusable for any external provider and must describe reads, writes, states, and scheduling, not Streamline implementation details.
- Temporal implementation details stay in temporal frame systems. Frame assembly may build or pass temporal state, but should not calculate jitter, history validity, provider resets, or motion-vector scaling inline.

## SDK Calls To Account For

Minimum Streamline call sequence for the production path:

1. `slInit` with preferences, log callback, application identifiers, requested feature list, and platform-specific binary path policy.
2. `slSetD3DDevice` for D3D12 or `slSetVulkanInfo` for Vulkan after RHI device creation.
3. `slGetFeatureRequirements` and `slIsFeatureSupported` for DLSS before enabling the provider.
4. `slDLSSGetOptimalSettings` when quality mode, display extent, or dynamic resolution policy changes.
5. `slSetTagForFrame` for color input, color output, depth, motion vectors, and optional exposure.
6. `slDLSSSetOptions` for quality mode, output extent, sharpness, reset, HDR, and exposure settings.
7. `slSetConstants` for per-frame/per-view temporal data.
8. `slEvaluateFeature` on the command list at the FrameGraph provider pass point.
9. Streamline resource cleanup calls when DLSS is disabled, resized, or the device is destroyed, followed by `slShutdown` during renderer shutdown.

## Runtime Dependencies

- Streamline SDK headers and import libraries must be treated as third-party dependencies, not copied into renderer source.
- Streamline runtime DLLs/plugins must be packaged beside the executable or in a documented runtime search path for editor/game configurations.
- GPU and driver support must be checked at runtime. Unsupported devices must use the fallback path without failing renderer initialization.
- Vulkan integration must choose between Streamline interposer and manual hooking. Sparkle should prefer a deliberate RHI-owned manual hook path if it keeps device/extension ownership clearer, but this needs validation in the RHI phase.

## Backend Support Decision

- D3D12 is the first production target because Sparkle already exposes native D3D12 device, queue, resource, and command-list handles through RHI surfaces that map cleanly to Streamline setup and evaluation.
- Vulkan remains an intended target, but it needs a specific Streamline integration choice: automatic interposer versus manual function-pointer hooking. This belongs in the RHI capability phase before provider code is written.
- Any backend without required native handle access, command-list evaluation support, resource tagging support, and validated Streamline feature support is unsupported for DLSS and must route through fallback.
- Unsupported backend status is a runtime capability result, not a renderer initialization failure.

## Fallback Path

If Streamline, DLSS, the GPU, the driver, required resources, or required runtime binaries are unavailable, Sparkle should:

- Log a clear one-time diagnostic with the missing requirement.
- Disable the DLSS provider for the viewport.
- Route the existing scene color into the final scene color product without allocating DLSS resources.
- Keep temporal state valid for downstream denoisers and any non-DLSS temporal passes.

## Architectural Gaps To Close Before Coding DLSS

- Add an early renderer/RHI bootstrap point that can satisfy Streamline initialization ordering.
- Add an RHI capability surface for external upscalers, native handles, backend support, and required resource state capabilities.
- Add a FrameGraph external-provider pass shape with explicit resource declarations and command-list state restoration rules.
- Split render extent and output extent as first-class frame concepts.
- Formalize motion-vector convention and conversion constants for providers.
- Add a renderer-owned final scene color product so consumers do not know whether native rendering, DLSS, or fallback produced it.
- Add SDK dependency acquisition, binary packaging, editor/game runtime lookup, and diagnostics.

## Phase 8.2.1 Validation

- Documentation review only. No build is required for this phase.
- Cross-check completed against NVIDIA Streamline, Streamline DLSS guide, Streamline Sample, `vk_streamline`, and NVIDIA DLSS programming guide references listed above.
- No renderer, RHI, shader, build, or runtime source code changes are required by this phase.
