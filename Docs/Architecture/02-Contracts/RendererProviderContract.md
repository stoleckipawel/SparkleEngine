# Renderer Provider Contract

## Purpose

This document defines the architecture contract for renderer-side SDK providers such as DLSS, Streamline-backed integrations, future FidelityFX integrations, denoisers, frame generation, ray tracing extensions, and future neural rendering providers.

The goal is to keep provider integrations isolated, capability-gated, diagnosable, and easy to replace without letting any single SDK define SparkleEngine renderer architecture.

This is a contract and boundary document. It describes how providers should fit into the engine. It does not add a new provider implementation.

## Non-Goals

- This document does not add FidelityFX or any new provider implementation.
- This document does not change current Streamline or DLSS behavior.
- This document does not turn provider-native API access into a general Renderer privilege.
- This document does not treat Streamline or DLSS as the renderer architecture itself.
- This document does not replace the RHI contract or frame-graph contract.

## Current Source-Backed Architecture Shape

The current source already shows the intended split:

- `SparkleRendererNvidiaDlssProvider` is a dedicated provider target, separate from the main `SparkleRenderer` target.
- `IUpscalerProvider` defines a renderer-owned provider interface.
- `UpscalerSubsystem` owns provider selection, initialization, per-frame setup, evaluation, resize, history reset, and shutdown.
- `RhiCapabilities` and `RhiInteropService` provide backend and native interop facts to the provider.
- Launcher dependency reporting already groups NVIDIA Streamline/NVAPI as optional source dependencies and uses host GPU detection to explain why that group is enabled or skipped.

Contract rule:

- Provider code lives behind renderer-owned provider interfaces and provider-scoped build targets.
- Renderer architecture stays provider-neutral even when only one provider exists today.

## Provider Categories

This contract applies to the following provider categories:

- upscaler
- denoiser
- frame generation
- ray tracing extension
- neural rendering

Category intent:

- `upscaler`: image reconstruction or native-AA style output generation from renderer-owned inputs
- `denoiser`: temporal or spatial reconstruction of lighting, shadow, GI, or neural outputs
- `frame generation`: synthesis of presentation frames from renderer-owned history and motion data
- `ray tracing extension`: vendor-specific acceleration or enhancement layered on top of renderer/RHI ray tracing contracts
- `neural rendering`: neural shading, inference-assisted reconstruction, neural materials, or related provider paths

Contract rule:

- categories may share common resource contracts and capability-state vocabulary
- category-specific behavior must still remain scoped to provider implementations and renderer-owned category contracts

## Provider Lifecycle

The current upscaler implementation provides the reference lifecycle shape:

1. capability query
2. provider selection
3. initialize
4. setup frame
5. evaluate
6. resize handling
7. history reset
8. diagnostics query
9. shutdown

### 1. Capability Query

Providers inspect:

- backend API
- adapter identity
- RHI external feature interop capabilities
- SDK integration/runtime availability
- provider-specific feature support

Current source-backed examples:

- `NvidiaDlssUpscalerProvider::QueryCapabilities(...)`
- `DlssCapabilityReporter::Build(...)`

### 2. Provider Selection

Renderer-owned orchestration chooses the requested or fallback provider.

Current source-backed owner:

- `UpscalerSubsystem`

Contract rule:

- the subsystem selects the provider
- the provider does not self-elect through hidden global checks

### 3. Initialize

Initialization receives:

- `RhiCapabilities`
- `RhiNativeDeviceQueueInterop`
- provider-specific presentation bridge when required

Contract rule:

- initialization must not bypass RHI interop policy
- initialization failure must remain diagnosable and must not collapse into silent renderer behavior

### 4. Setup Frame

Frame setup receives a renderer-owned input contract before evaluation.

Current source-backed example:

- `UpscalerInputContract`

Contract rule:

- all provider-required frame inputs should arrive through explicit contracts
- providers should not scrape ad hoc renderer globals to find missing data

### 5. Evaluate

Evaluation runs the provider on concrete frame resources and may produce:

- output produced
- fallback used
- failure domain
- reason

Current source-backed example:

- `UpscalerEvaluationResult`

### 6. Resize Handling

Providers must be informed when render/output extents change.

Current source-backed example:

- `OnResize(renderExtent, outputExtent)`

### 7. History Reset

Providers must accept renderer-owned reset requests and reasons.

Current source-backed example:

- `ResetHistory(reason)`

### 8. Diagnostics Query

Providers must expose structured runtime diagnostics.

Current source-backed examples:

- `UpscalerProviderCapabilities`
- `DlssCapabilityReport`
- `StreamlineDlssRuntimeDiagnostics`

### 9. Shutdown

Providers must explicitly release runtime state.

Contract rule:

- provider shutdown should be explicit and idempotent

## Capability States

All renderer providers must map their internal status model to these six capability states exactly:

- `unavailable`
- `missing dependency`
- `unsupported hardware`
- `available`
- `enabled`
- `runtime failed`

These names are the cross-provider architectural vocabulary. Provider-specific enums may be richer internally, but any renderer-facing or launcher-facing contract should map back to these six states.

### `unavailable`

Use when the provider cannot be used and the reason is not specifically a missing dependency or unsupported hardware.

Examples:

- backend bridge kind is not accepted
- required interop features are not exposed
- provider is not selected by policy

### `missing dependency`

Use when required SDK files, runtime binaries, source dependencies, or host-side prerequisites are absent.

Examples:

- Streamline SDK not synced
- NVAPI source dependency not present
- required runtime DLLs unavailable
- required Vulkan SDK host dependency missing for the provider build path

### `unsupported hardware`

Use when the dependency exists but the selected adapter or backend cannot support the feature.

Examples:

- provider feature query reports unsupported adapter
- selected GPU vendor does not support the provider feature
- backend lacks the provider-required bridge or feature tier for the active hardware path

### `available`

Use when all prerequisites are satisfied and the provider could be initialized or created, but it is not currently active.

Examples:

- capability query succeeded
- feature can be created
- provider is ready but not selected yet

### `enabled`

Use when the provider is selected and active for the current runtime path.

Examples:

- initialized and ready for evaluation
- actively evaluating frames

### `runtime failed`

Use when the provider was selected or initialized, but runtime execution failed and the engine has moved to a guarded fallback path or disabled state.

Examples:

- initialization failed after selection
- input contract invalid at runtime
- evaluation failed and deterministic fallback was used

Contract rule:

- renderer-facing docs, diagnostics, and launcher reporting should use these six states consistently, even when an internal provider enum has more detail such as `Created`, `Evaluating`, or `FailedWithFallback`

## Upscaler Resource Contract Table

This table defines the renderer vocabulary image upscalers must use. Denoisers and reconstruction providers use a separate resource surface so upscaling does not become a catch-all provider category.

| Resource | Contract meaning | Current source evidence | Provider expectation |
| --- | --- | --- | --- |
| scaling input color | Main provider input color, typically scene color before upscaler evaluation | `UpscalerInputContract.ScalingInputColor`, `UpscalerEvaluationDesc.ScalingInputColor` | Required for image upscalers |
| scaling output color | Provider output color target | `UpscalerInputContract.ScalingOutputColor`, `UpscalerEvaluationDesc.ScalingOutputColor` | Required for image upscalers that write an output color product |
| depth | Main scene depth with declared convention | `UpscalerInputContract.Depth`, `UpscalerEvaluationDesc.Depth`, `UpscalerDepthConvention` | Required for temporal upscaler reprojection |
| motion vectors | Renderer-owned motion vectors with explicit units/direction contract | `UpscalerInputContract.MotionVectors`, motion-vector convention enums, `UpscalerEvaluationDesc.MotionVectors` | Required for temporal upscaler reprojection |
| exposure | Optional or required exposure resource depending on provider category | `UpscalerInputContract.Exposure`, `ExposureRequired` | Must be explicit when needed; providers must not infer hidden exposure sources |
| history | Temporal validity and provider-managed history state | `HistoryInvalid`, `ResetRequested`, `ResetReason`, `TemporalState.HistoryValid`, provider `ResetHistory(...)` | Providers must accept renderer-owned history invalidation and must not hide global reset policy |
| jitter | Current and previous jitter used for temporal alignment | `PerTemporalConstantBufferData`, `TemporalData`, `TemporalState` | Providers must consume renderer-supplied jitter data rather than generating a separate independent temporal sequence |
| camera matrices | Renderer-owned per-view camera state | `UpscalerInputContract.Camera`, `PerViewCameraConstantBufferData` | Required for providers that need reprojection, clip-space transforms, or neural camera features |
| frame index | Stable renderer-owned frame identifier | `UpscalerInputContract.FrameIndex`, `UpscalerEvaluationDesc.FrameIndex` | Required for deterministic temporal sequencing and diagnostics |

## Denoiser And Reconstruction Resource Surface

These resources are deliberately not part of `UpscalerInputContract`. They are frame-level denoiser/reconstruction inputs, currently collected by `FrameAssemblyDenoiserProviderResources` and consumed by future denoiser/reconstruction stages.

| Resource | Contract meaning | Current source evidence | Provider expectation |
| --- | --- | --- | --- |
| normals | Renderer-owned surface normal resource | `FrameAssemblyDenoiserProviderResources.Normals`, `GBufferNormal` | Required for denoisers, some neural paths, and any provider that depends on geometric surface orientation |
| noisy indirect diffuse | Linear HDR material-evaluated indirect diffuse radiance contribution | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.NoisyIndirectDiffuse`, `LightingRenderTargets.IndirectDiffuse` | Optional guide for indirect reconstruction providers; not a substitute for provider-ready albedo or hit-distance guides |
| noisy indirect specular | Linear HDR material-evaluated indirect specular radiance contribution | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.NoisyIndirectSpecular`, `LightingRenderTargets.IndirectSpecular` | Optional guide for indirect reconstruction providers and reflection reconstruction |
| albedo | Provider-ready diffuse reflectance guide, linear, material evaluated | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.Albedo` | Required by DLRR-style reconstruction when selected; plain `GBufferBaseColor` is not enough for metallic materials |
| specular albedo | Provider-ready specular reflectance/F0 guide, linear, material evaluated | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.SpecularAlbedo` | Required by DLRR-style reconstruction when selected; must be derived from the same F0/metallic policy used by lighting |
| roughness | Linear material roughness guide in `[0, 1]` | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.Roughness` | Required by DLRR-style reconstruction when selected; packed GBuffer material data needs an explicit resolve before a provider can consume it |
| diffuse hit distance | First-bounce diffuse/indirect ray hit distance in world units | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.DiffuseHitDistance` | Optional for providers that reconstruct diffuse/GI rays |
| specular hit distance | First-bounce specular ray hit distance in world units | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.SpecularHitDistance` | Required by DLRR-style reconstruction unless `SpecularMotionVectors` is provided |
| specular motion vectors | Dense reflection motion-vector guide | `FrameAssemblyDenoiserProviderResources.IndirectReconstruction.SpecularMotionVectors` | Streamline-compatible alternative to specular hit distance for reflection reconstruction |

Contract rules:

1. Required resources must be named and passed through explicit provider contracts.
2. Providers must not read hidden renderer globals to recover missing resources.
3. A provider may mark some resources optional for its category, but that decision must stay provider-scoped and visible.
4. If a resource is required by the provider, contract validation must fail clearly when it is missing.

## Backend And Native Handle Policy

Provider-native API usage must remain backend-scoped and provider-scoped.

Current source-backed boundaries:

- Renderer native API exceptions are already tracked through boundary rules and ADRs.
- DLSS/Streamline is isolated in a dedicated provider target.
- RHI exposes native handles and bridge facts through interop types, not through arbitrary renderer-wide access.

Contract rules:

1. Native API usage must be backend/provider-scoped and documented through boundary rules and ADRs.
2. Provider-native details must not become general permission for unrelated renderer passes or systems.
3. A provider may consume native handles only through RHI interop surfaces and provider-scoped contracts.
4. If a provider needs a new native escape hatch, that change must be documented as an architecture exception before it spreads.

Relevant current docs:

- [BoundaryRules.md](../01-Boundaries/BoundaryRules.md)
- [0001-renderer-native-api-provider-exceptions.md](../01-Boundaries/ADR/0001-renderer-native-api-provider-exceptions.md)

## RHI Interop Policy

RHI owns native-handle exposure and interop capability facts.

Current source-backed interop surfaces:

- `RhiInteropService`
- `RhiNativeDeviceQueueInterop`
- `RhiNativeInteropRequest`
- `RhiNativeHandles`
- `NativeTextureViewInfo`

Current source-backed interop consumers include:

- `Validation`
- `UpscalerProvider`
- `RendererFrameGraph`
- `PresentationBridge`

Contract rules:

1. Providers request interop from RHI; they do not discover backend-native objects by other means.
2. Interop requests should declare a consumer and reason.
3. Providers must treat `RhiCapabilities.ExternalFeatureInterop` as authoritative capability fact input.
4. Providers should only depend on interop fields they explicitly require.
5. Missing interop capability should map to a capability state and diagnostic reason, not undefined behavior.

Interop implication for reviewers:

- the RHI remains the owner of backend-native object exposure policy
- the provider remains the owner of SDK translation logic
- the renderer remains the owner of provider orchestration and resource contracts

## Error Reporting Policy

Providers must produce explicit failure reason and failure domain reporting.

Current source-backed patterns:

- `UpscalerProviderCapabilities.Reason`
- `UpscalerEvaluationResult.Reason`
- `UpscalerProviderFailureDomain`
- `DlssCapabilityReport.UnavailableReason`
- `StreamlineDlssRuntimeDiagnostics.FailureReason`

Contract rules:

1. Capability failure and runtime failure must be distinguishable.
2. Dependency failure, hardware failure, backend failure, resource-state failure, and input-contract failure should remain separate when the source can tell them apart.
3. Runtime failure must surface a stable fallback reason when fallback occurs.
4. Provider diagnostics should be reviewer-readable and stable enough for automated validation output.

## Debug UI And Diagnostics Expectations

Provider diagnostics should be visible without reading provider source files.

Current source-backed signals already available:

- provider name
- external runtime version
- runtime state
- feature matrix summary
- selected quality mode
- render/output extents
- reset requests and reset reason
- failure reason

Contract expectations:

1. Renderer or editor diagnostics should expose active provider kind and the mapped capability state.
2. Diagnostics should show whether the provider uses external SDK files.
3. Diagnostics should show why a provider is unavailable, dependency-blocked, unsupported, enabled, or runtime-failed.
4. Provider-specific feature-matrix details may appear in deeper diagnostics, but the top-level UI should still map to the common provider vocabulary.
5. Debug UI should be able to answer:
   - what provider was requested
   - what provider is active
   - why a fallback was used
   - what required resource or dependency was missing

## Dependency Sync And Launcher Reporting Expectations

Launcher and dependency reporting should describe provider readiness without redefining renderer policy.

Current source-backed launcher behavior:

- source dependencies are grouped through `SourceDependencyGroup`
- `nvidia-streamline` is an optional dependency group
- the group summary explicitly mentions DLSS, NVAPI, Streamline staging, PTLAS integration, and Vulkan SDK expectation
- host GPU capability detection reports whether NVIDIA, AMD, or Intel adapters were detected
- group UI distinguishes disabled, ready, cached, partial, pending sync, and available states

Contract rules:

1. Launcher reporting should distinguish hardware-driven disablement from missing cached dependencies.
2. Optional provider groups should explain why they are enabled or skipped.
3. Dependency sync reporting should name the specific provider dependency group, not bury provider readiness inside generic third-party state.
4. Launcher should report missing provider dependencies as dependency issues, not as renderer architecture failures.
5. Host GPU detection may influence default enablement, but it must not become the only source of truth for runtime capability.
6. Launcher messaging should remain consistent with provider capability states:
   - missing synced SDK files maps to `missing dependency`
   - unsupported adapter maps to `unsupported hardware`
   - ready-to-use provider dependencies maps to `available`

## New Provider Checklist

Use this checklist before adding a new provider implementation.

1. Choose the provider category.
   - upscaler
   - denoiser
   - frame generation
   - ray tracing extension
   - neural rendering

2. Define the renderer-owned provider interface surface.
   - do not let the SDK surface become the engine interface

3. Define capability mapping to the six architectural states exactly.
   - `unavailable`
   - `missing dependency`
   - `unsupported hardware`
   - `available`
   - `enabled`
   - `runtime failed`

4. Define the required resource contract.
   - color
   - depth
   - motion vectors
   - exposure
   - normals
   - history
   - jitter
   - camera matrices
   - frame index

5. Define validation behavior.
   - what is required at capability-query time
   - what is required at initialization time
   - what is required at frame-setup time
   - what causes runtime fallback

6. Define RHI interop needs.
   - native device
   - queue
   - command list or command buffer
   - native resource handles
   - texture view info
   - explicit resource-state expectations

7. Confirm boundary status.
   - if native backend API use is required, ensure it is provider-scoped
   - add or update boundary-rule documentation
   - add or update ADRs for exceptions

8. Add provider diagnostics.
   - provider name
   - version
   - mapped capability state
   - failure domain
   - reason
   - requested versus active mode

9. Add launcher/dependency reporting.
   - source dependency group if external SDK files are needed
   - host capability explanation where useful
   - dependency validation paths

10. Keep fallback deterministic.
   - provider failure must degrade to a known renderer path without hidden global state

11. Document known limits honestly.
   - backend gaps
   - hardware gaps
   - SDK staging gaps
   - validation gaps

## Known Gaps

- The current provider-neutral architecture is strongest for upscalers; equivalent first-class renderer interfaces for denoisers, frame generation, ray tracing extensions, and neural rendering are still planned rather than fully implemented.
- The common architectural capability states in this document are broader than the current `EUpscalerProviderStatus` enum and would need an explicit shared mapping layer if the engine wants all providers to report identically.
- Normals are clearly part of the renderer frame-resource vocabulary, but they are not yet part of the current upscaler input contract surface.
- Exposure is present in `UpscalerInputContract`, but the reviewed source currently defaults `ExposureRequired` to false and does not yet show a broad provider-wide exposure pipeline.
- Launcher dependency states and renderer provider capability states are related but not yet unified in one shared vocabulary.

## Source Anchors

Primary reviewed files for this contract:

- `Docs/Architecture/00-Review/A_PrincipalRoleRequirements.md`
- `Docs/Architecture/00-Review/B_EngineArchitectureScorecard.md`
- `Engine/Renderer/CMakeLists.txt`
- `Engine/Renderer/Private/Upscaling/UpscalerProvider.h`
- `Engine/Renderer/Private/Upscaling/UpscalerInputContract.h`
- `Engine/Renderer/Private/Upscaling/UpscalerInputContract.cpp`
- `Engine/Renderer/Private/Upscaling/UpscalerInputContractBuilder.h`
- `Engine/Renderer/Private/Upscaling/UpscalerInputContractBuilder.cpp`
- `Engine/Renderer/Private/Upscaling/UpscalerSubsystem.h`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.cpp`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss/DlssCapabilityReport.h`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss/DlssCapabilityReport.cpp`
- `Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.h`
- `Engine/RHI/Public/Interop/RhiNativeHandles.h`
- `Engine/RHI/Public/Interop/RhiInteropService.h`
- `Tools/Launcher/SparkleLauncher/Public/SparkleLauncher/SourceDependencyState.h`
- `Tools/Launcher/SparkleLauncher/Private/Core/SourceDependencyState.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Core/HostGraphicsCapabilities.h`
- `Tools/Launcher/SparkleLauncher/Private/Core/HostGraphicsCapabilities.cpp`
- `Tools/Launcher/SparkleLauncher/Private/Gui/Models/LauncherDependencyUiModel.cpp`
